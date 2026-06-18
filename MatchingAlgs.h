#pragma once

#include "pch.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

using namespace std;
using namespace cv;

// ============================================================
// 1. StitchingContext：数据上下文
// ============================================================
class StitchingContext {
public:
    // 输入
    Mat imgTemplate;
    Mat imgScene;
    string algorithmType = "SIFT";
    bool enableNMS = true;

    // 第1步：特征点
    vector<KeyPoint> kpsTemplate;
    vector<KeyPoint> kpsScene;
    Mat descTemplate;
    Mat descScene;

    // 第2步：特征匹配
    vector<DMatch> allMatches;
    vector<DMatch> goodMatches;

    // 第3步：单应性矩阵
    vector<Point2f> ptsTemplate;
    vector<Point2f> ptsScene;
    vector<uchar> inliersMask;
    Mat H;

    // 最终结果（第4步展示）
    Mat resultImg;

    // 统计数据
    long executionTimeMs = 0;
    int totalInitialMatches = 0;
    int matchedPointsCount = 0;
    double accuracyScore = 0.0;

    // 步骤可视化图像（分别存放左右图，用于单独显示）
    Mat visLeft;   // 模板图特征/匹配等
    Mat visRight;  // 场景图特征/匹配等

    // 合并图（用于 Step3、Step4 的 drawMatches 结果）
    Mat stepImage_Matches;
    Mat stepImage_Homography;

    void clearMiddleResults() {
        kpsTemplate.clear();
        kpsScene.clear();
        descTemplate.release();
        descScene.release();
        allMatches.clear();
        goodMatches.clear();
        ptsTemplate.clear();
        ptsScene.clear();
        inliersMask.clear();
        H.release();
        resultImg.release();
        visLeft.release();
        visRight.release();
        stepImage_Matches.release();
        stepImage_Homography.release();
    }
};

// ============================================================
// 2. IStep：步骤接口
// ============================================================
class IStep {
public:
    virtual ~IStep() = default;
    virtual bool execute(StitchingContext& ctx) = 0;
    virtual string getStepName() = 0;
    virtual Mat getVisualization(StitchingContext& ctx) = 0;
    // 新增方法：用于 Step1/Step2 分别获取左右图
    virtual Mat getLeftVisualization(StitchingContext& ctx) { return ctx.visLeft; }
    virtual Mat getRightVisualization(StitchingContext& ctx) { return ctx.visRight; }
};

// ============================================================
// 3. Step1：检测特征点
// ============================================================
class Step1_DetectKeypoints : public IStep {
public:
    bool execute(StitchingContext& ctx) override {
        Mat grayTemplate, grayScene;
        cvtColor(ctx.imgTemplate, grayTemplate, COLOR_BGR2GRAY);
        cvtColor(ctx.imgScene, grayScene, COLOR_BGR2GRAY);

        if (ctx.algorithmType == "SIFT") {
            Ptr<SIFT> detector = SIFT::create();
            detector->detect(grayTemplate, ctx.kpsTemplate);
            detector->detect(grayScene, ctx.kpsScene);
        }
        else if (ctx.algorithmType == "ORB") {
            Ptr<ORB> detector = ORB::create();
            detector->detect(grayTemplate, ctx.kpsTemplate);
            detector->detect(grayScene, ctx.kpsScene);
        }
        else {
            return false;
        }

        if (ctx.enableNMS) {
            float radiusT = sqrt(ctx.imgTemplate.cols * ctx.imgTemplate.cols +
                ctx.imgTemplate.rows * ctx.imgTemplate.rows) * 0.02f;
            float radiusS = sqrt(ctx.imgScene.cols * ctx.imgScene.cols +
                ctx.imgScene.rows * ctx.imgScene.rows) * 0.02f;
            applyNMS(ctx.kpsTemplate, radiusT);
            applyNMS(ctx.kpsScene, radiusS);
        }

        // 分别生成左右图
        generateVisualization(ctx);
        return true;
    }

    string getStepName() override { return "特征点检测"; }

    Mat getVisualization(StitchingContext& ctx) override {
        return ctx.visLeft; // 返回左图（实际不会用到，main中会分别显示）
    }

    Mat getLeftVisualization(StitchingContext& ctx) override {
        return ctx.visLeft;
    }

    Mat getRightVisualization(StitchingContext& ctx) override {
        return ctx.visRight;
    }

private:
    void applyNMS(vector<KeyPoint>& keypoints, float radius) {
        sort(keypoints.begin(), keypoints.end(),
            [](const KeyPoint& a, const KeyPoint& b) { return a.response > b.response; });
        vector<bool> suppressed(keypoints.size(), false);
        vector<KeyPoint> kept;
        for (size_t i = 0; i < keypoints.size(); ++i) {
            if (suppressed[i]) continue;
            kept.push_back(keypoints[i]);
            for (size_t j = i + 1; j < keypoints.size(); ++j) {
                if (suppressed[j]) continue;
                if (norm(keypoints[i].pt - keypoints[j].pt) < radius) {
                    suppressed[j] = true;
                }
            }
        }
        keypoints = kept;
    }

    void generateVisualization(StitchingContext& ctx) {
        drawKeypoints(ctx.imgTemplate, ctx.kpsTemplate, ctx.visLeft, Scalar(0, 255, 0));
        drawKeypoints(ctx.imgScene, ctx.kpsScene, ctx.visRight, Scalar(0, 255, 0));
    }
};

// ============================================================
// 4. Step2：计算描述子
// ============================================================
class Step2_ComputeDescriptors : public IStep {
public:
    bool execute(StitchingContext& ctx) override {
        Mat grayTemplate, grayScene;
        cvtColor(ctx.imgTemplate, grayTemplate, COLOR_BGR2GRAY);
        cvtColor(ctx.imgScene, grayScene, COLOR_BGR2GRAY);

        if (ctx.algorithmType == "SIFT") {
            Ptr<SIFT> detector = SIFT::create();
            detector->compute(grayTemplate, ctx.kpsTemplate, ctx.descTemplate);
            detector->compute(grayScene, ctx.kpsScene, ctx.descScene);
        }
        else if (ctx.algorithmType == "ORB") {
            Ptr<ORB> detector = ORB::create();
            detector->compute(grayTemplate, ctx.kpsTemplate, ctx.descTemplate);
            detector->compute(grayScene, ctx.kpsScene, ctx.descScene);
        }
        else {
            return false;
        }
        // 可视化沿用 Step1 的特征点图（描述子不可视）
        drawKeypoints(ctx.imgTemplate, ctx.kpsTemplate, ctx.visLeft, Scalar(0, 255, 0));
        drawKeypoints(ctx.imgScene, ctx.kpsScene, ctx.visRight, Scalar(0, 255, 0));
        return true;
    }

    string getStepName() override { return "计算描述子"; }

    Mat getVisualization(StitchingContext& ctx) override {
        return ctx.visLeft;
    }

    Mat getLeftVisualization(StitchingContext& ctx) override {
        return ctx.visLeft;
    }

    Mat getRightVisualization(StitchingContext& ctx) override {
        return ctx.visRight;
    }
};

// ============================================================
// 5. Step3：特征匹配
// ============================================================
class Step3_MatchFeatures : public IStep {
public:
    bool execute(StitchingContext& ctx) override {
        if (ctx.descTemplate.empty() || ctx.descScene.empty()) return false;

        if (ctx.algorithmType == "SIFT") {
            Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
            vector<vector<DMatch>> knnMatches;
            matcher->knnMatch(ctx.descTemplate, ctx.descScene, knnMatches, 2);

            ctx.allMatches.clear();
            ctx.goodMatches.clear();
            for (auto& km : knnMatches) {
                if (km.size() >= 2 && km[0].distance < 0.75f * km[1].distance) {
                    ctx.goodMatches.push_back(km[0]);
                }
            }
        }
        else if (ctx.algorithmType == "ORB") {
            Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::BRUTEFORCE_HAMMING);
            vector<vector<DMatch>> knnMatches;
            matcher->knnMatch(ctx.descTemplate, ctx.descScene, knnMatches, 2);

            ctx.allMatches.clear();
            ctx.goodMatches.clear();
            for (auto& km : knnMatches) {
                if (km.size() >= 2 && km[0].distance < 0.75f * km[1].distance) {
                    ctx.goodMatches.push_back(km[0]);
                }
            }
        }
        else {
            return false;
        }

        ctx.stepImage_Matches = generateVisualization(ctx);
        return true;
    }

    string getStepName() override { return "特征匹配"; }

    Mat getVisualization(StitchingContext& ctx) override {
        return ctx.stepImage_Matches;
    }

private:
    Mat generateVisualization(StitchingContext& ctx) {
        Mat vis;
        drawMatches(ctx.imgTemplate, ctx.kpsTemplate,
            ctx.imgScene, ctx.kpsScene,
            ctx.goodMatches, vis,
            Scalar::all(-1), Scalar::all(-1),
            vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
        return vis;
    }
};

// ============================================================
// 6. Step4：计算单应性矩阵（最终结果）
// ============================================================
class Step4_ComputeHomography : public IStep {
public:
    bool execute(StitchingContext& ctx) override {
        if (ctx.goodMatches.size() < 4) {
            cout << "匹配点不足4个，无法计算单应性矩阵" << endl;
            return false;
        }

        ctx.ptsTemplate.clear();
        ctx.ptsScene.clear();
        for (auto& match : ctx.goodMatches) {
            ctx.ptsTemplate.push_back(ctx.kpsTemplate[match.queryIdx].pt);
            ctx.ptsScene.push_back(ctx.kpsScene[match.trainIdx].pt);
        }

        ctx.H = findHomography(ctx.ptsTemplate, ctx.ptsScene, RANSAC, 3.0, ctx.inliersMask);
        if (ctx.H.empty()) {
            cout << "单应性矩阵计算失败" << endl;
            return false;
        }

        int inliersCount = 0;
        for (uchar m : ctx.inliersMask) {
            if (m == 1) inliersCount++;
        }
        ctx.matchedPointsCount = inliersCount;
        ctx.totalInitialMatches = (int)ctx.goodMatches.size();
        ctx.accuracyScore = (static_cast<double>(inliersCount) / ctx.totalInitialMatches) * 100.0;

        ctx.stepImage_Homography = generateVisualization(ctx);
        ctx.resultImg = ctx.stepImage_Homography.clone();
        return true;
    }

    string getStepName() override { return "单应性矩阵计算（匹配结果）"; }

    Mat getVisualization(StitchingContext& ctx) override {
        return ctx.stepImage_Homography;
    }

private:
    Mat generateVisualization(StitchingContext& ctx) {
        Mat vis;
        // 将 inliersMask 从 vector<uchar> 转为 vector<char>
        vector<char> matchesMask(ctx.inliersMask.begin(), ctx.inliersMask.end());

       

        // 绘制四个角点映射边框
        vector<Point2f> corners(4);
        corners[0] = Point2f(0, 0);
        corners[1] = Point2f((float)ctx.imgTemplate.cols, 0);
        corners[2] = Point2f((float)ctx.imgTemplate.cols, (float)ctx.imgTemplate.rows);
        corners[3] = Point2f(0, (float)ctx.imgTemplate.rows);

        vector<Point2f> sceneCorners(4);
        perspectiveTransform(corners, sceneCorners, ctx.H);


        for (int i = 0; i < 4; i++) {
            line(ctx.visRight, sceneCorners[i], sceneCorners[(i + 1) % 4], Scalar(0, 255, 0), 3);
        }

        string text = "准确率: " + to_string(ctx.accuracyScore).substr(0, 5) + "%";
        putText(ctx.visRight, text, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 0, 0), 2);

        string info = "内点: " + to_string(ctx.matchedPointsCount) + "/" + to_string(ctx.totalInitialMatches);
        putText(ctx.visRight, info, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 0, 0), 2);

        return ctx.visRight;
    }
};

// ============================================================
// 7. StitchController：流程控制器（只含4步）
// ============================================================
class StitchController {
public:
    StitchController() : m_currentStep(-1) {}

    void setAlgorithm(const string& type, bool enableNMS = true) {
        m_context.algorithmType = type;
        m_context.enableNMS = enableNMS;
        m_context.clearMiddleResults();
        m_currentStep = -1;

        m_steps.clear();
        m_steps.push_back(make_shared<Step1_DetectKeypoints>());
        m_steps.push_back(make_shared<Step2_ComputeDescriptors>());
        m_steps.push_back(make_shared<Step3_MatchFeatures>());
        m_steps.push_back(make_shared<Step4_ComputeHomography>());
    }

    void setImages(const Mat& templateImg, const Mat& sceneImg) {
        m_context.imgTemplate = templateImg.clone();
        m_context.imgScene = sceneImg.clone();
        m_context.clearMiddleResults();
        m_currentStep = -1;
    }

    bool executeNextStep() {
        int nextStep = m_currentStep + 1;
        if (nextStep >= (int)m_steps.size()) {
            return false;
        }

        auto start = chrono::high_resolution_clock::now();
        bool success = m_steps[nextStep]->execute(m_context);
        auto end = chrono::high_resolution_clock::now();

        if (success) {
            m_currentStep = nextStep;
            m_context.executionTimeMs += chrono::duration_cast<chrono::milliseconds>(end - start).count();
        }
        return success;
    }

    void executePrevStep() {
        if (m_currentStep > 0) {
            m_currentStep--;
        }
    }

    Mat getCurrentStepImage() {
        if (m_currentStep < 0 || m_currentStep >= (int)m_steps.size()) {
            return Mat();
        }
        return m_steps[m_currentStep]->getVisualization(m_context);
    }

    // 获取左右图（用于 Step1/Step2）
    Mat getCurrentLeftImage() {
        if (m_currentStep < 0 || m_currentStep >= (int)m_steps.size()) {
            return Mat();
        }
        return m_steps[m_currentStep]->getLeftVisualization(m_context);
    }

    Mat getCurrentRightImage() {
        if (m_currentStep < 0 || m_currentStep >= (int)m_steps.size()) {
            return Mat();
        }
        return m_steps[m_currentStep]->getRightVisualization(m_context);
    }

    string getCurrentStepName() {
        if (m_currentStep < 0 || m_currentStep >= (int)m_steps.size()) {
            return "未开始";
        }
        return m_steps[m_currentStep]->getStepName();
    }

    int getCurrentStepIndex() { return m_currentStep + 1; }
    int getTotalSteps() { return (int)m_steps.size(); }
    bool isAllStepsDone() { return m_currentStep >= (int)m_steps.size() - 1; }
    StitchingContext& getContext() { return m_context; }

    void reset() {
        m_context.clearMiddleResults();
        m_currentStep = -1;
    }

private:
    StitchingContext m_context;
    vector<shared_ptr<IStep>> m_steps;
    int m_currentStep;
};

// ============================================================
// 8. 策略类（保留原有接口，兼容旧代码）
// ============================================================
class MatchResult {
public:
    string name;
    long executionTimeMs;
    double accuracyScore;
    int matchedPointsCount;
    int totalInitialMatches;
};

class ImageMatchingStrategy {
public:
    virtual MatchResult match(const Mat& source, const Mat& target) = 0;
};

class SiftMatchingStrategy : public ImageMatchingStrategy {
public:
    bool enableNMS = true;

    MatchResult match(const Mat& source, const Mat& target) override {
        auto start = chrono::high_resolution_clock::now();

        StitchController controller;
        controller.setImages(source, target);
        controller.setAlgorithm("SIFT", enableNMS);

        while (controller.executeNextStep()) {
            // 全部执行
        }

        auto end = chrono::high_resolution_clock::now();
        auto& ctx = controller.getContext();

        MatchResult res;
        res.name = "SIFT";
        res.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        res.totalInitialMatches = ctx.totalInitialMatches;
        res.matchedPointsCount = ctx.matchedPointsCount;
        res.accuracyScore = ctx.accuracyScore;

        imwrite("sift_final_result.jpg", ctx.stepImage_Homography);

        return res;
    }
};

class OrbMatchingStrategy : public ImageMatchingStrategy {
public:
    bool enableNMS = true;

    MatchResult match(const Mat& source, const Mat& target) override {
        auto start = chrono::high_resolution_clock::now();

        StitchController controller;
        controller.setImages(source, target);
        controller.setAlgorithm("ORB", enableNMS);

        while (controller.executeNextStep()) {
            // 全部执行
        }

        auto end = chrono::high_resolution_clock::now();
        auto& ctx = controller.getContext();

        MatchResult res;
        res.name = "ORB";
        res.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        res.totalInitialMatches = ctx.totalInitialMatches;
        res.matchedPointsCount = ctx.matchedPointsCount;
        res.accuracyScore = ctx.accuracyScore;

        imwrite("orb_final_result.jpg", ctx.stepImage_Homography);

        return res;
    }
};

class ImageMatchingContext {
public:
    vector<MatchResult> executeMatch(const Mat& img_object, const Mat& img_scene,
        vector<ImageMatchingStrategy*> strategies) {
        vector<MatchResult> results;
        for (auto* p : strategies) {
            results.push_back(p->match(img_object, img_scene));
        }
        return results;
    }
};

// ============================================================
// 9. 主函数
// ============================================================
//int algs_main(int argc, char** argv) {
//    // 修改成你的图片路径
//    Mat img_object = imread("C:\\Users\\HP\\Desktop\\Samples\\0.png");
//    Mat img_scene = imread("C:\\Users\\HP\\Desktop\\Samples\\1.png");
//
//    if (img_object.empty() || img_scene.empty()) {
//        cout << "无法读取图像，请检查文件路径！" << endl;
//        return -1;
//    }
//
//    cout << "===== 逐步展示匹配过程（共4步）=====" << endl;
//
//    StitchController controller;
//    controller.setImages(img_object, img_scene);
//    controller.setAlgorithm("SIFT", true);
//
//    while (controller.executeNextStep()) {
//        int stepIdx = controller.getCurrentStepIndex();
//        int total = controller.getTotalSteps();
//        string stepName = controller.getCurrentStepName();
//
//        cout << "执行步骤 " << stepIdx << "/" << total << ": " << stepName << endl;
//
//        // 判断步骤类型：Step1 和 Step2 分别显示左右图，Step3 和 Step4 显示合并图
//        if (stepIdx == 1 || stepIdx == 2) {
//            Mat left = controller.getCurrentLeftImage();
//            Mat right = controller.getCurrentRightImage();
//            if (!left.empty()) {
//                imshow("模板图 - " + stepName, left);
//            }
//            if (!right.empty()) {
//                imshow("场景图 - " + stepName, right);
//            }
//        }
//        else {
//            Mat vis = controller.getCurrentStepImage();
//            if (!vis.empty()) {
//                imshow("步骤 " + to_string(stepIdx) + ": " + stepName, vis);
//            }
//        }
//        waitKey(0);
//        destroyAllWindows();
//    }
//
//    // 打印最终结果
//    auto& ctx = controller.getContext();
//    cout << "\n===== 匹配完成 =====" << endl;
//    cout << "总耗时: " << ctx.executionTimeMs << "ms" << endl;
//    cout << "初始匹配数: " << ctx.totalInitialMatches << endl;
//    cout << "内点数: " << ctx.matchedPointsCount << endl;
//    cout << "准确率: " << ctx.accuracyScore << "%" << endl;
//
//    if (!ctx.stepImage_Homography.empty()) {
//        imwrite("final_result.jpg", ctx.stepImage_Homography);
//        cout << "结果图已保存: final_result.jpg" << endl;
//    }
//
//    waitKey(0);
//    return 0;
//}
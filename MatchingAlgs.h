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
// StitchingContext：数据上下文
// ============================================================
class StitchingContext {
public:
    // 输入
    Mat imgTemplate;
    Mat imgScene;
    string algorithmType = "SIFT";
    bool enableNMS = true;

    // 特征点
    vector<KeyPoint> kpsTemplate;
    vector<KeyPoint> kpsScene;
    Mat descTemplate;
    Mat descScene;

    // 特征匹配
    vector<DMatch> allMatches;
    vector<DMatch> goodMatches;
    vector<DMatch> inlierMatches;

    // 单应性矩阵
    vector<Point2f> ptsTemplate;
    vector<Point2f> ptsScene;
    Mat inliersMask;
    Mat H;

    // 最终结果
    Mat resultImg;

    // 统计数据
    long executionTimeMs = 0; // 每个步骤更新
    int totalInitialMatches = 0;
    int matchedPointsCount = 0;
    double accuracyScore = 0.0;

    // 步骤可视化图像（分别存放左右图，用于单独显示）
    Mat visLeft;   // 模板图特征/匹配等
    Mat visRight;  // 场景图特征/匹配等

    // 不在GUI中呈现
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
        //inliersMask.clear();
        H.release();
        resultImg.release();
        visLeft.release();
        visRight.release();
        stepImage_Matches.release();
        stepImage_Homography.release();
    }
};

// ============================================================
// IStep：步骤接口
// ============================================================
class IStep {
public:
    virtual ~IStep() = default;
    virtual bool execute(StitchingContext& ctx) = 0;
    virtual string getStepName() = 0;
    // 分别获取左右图
    virtual Mat getLeftVisualization(StitchingContext& ctx) { return ctx.visLeft; }
    virtual Mat getRightVisualization(StitchingContext& ctx) { return ctx.visRight; }
};

// ============================================================
// Step：检测特征点
// ============================================================
class Step_DetectKeypoints : public IStep {
public:
    bool execute(StitchingContext& ctx) override {
        // 开始计时
        auto startTime = chrono::high_resolution_clock::now();
        Mat grayTemplate, grayScene;
        cvtColor(ctx.imgTemplate, grayTemplate, COLOR_BGR2GRAY);
        cvtColor(ctx.imgScene, grayScene, COLOR_BGR2GRAY);

        if (ctx.algorithmType == "SIFT") {
            Ptr<SIFT> detector = SIFT::create();
            detector->detect(grayTemplate, ctx.kpsTemplate);
            detector->detect(grayScene, ctx.kpsScene);
        }
        else { // ORB
            Ptr<ORB> detector = ORB::create();
            detector->detect(grayTemplate, ctx.kpsTemplate);
            detector->detect(grayScene, ctx.kpsScene);
        }

        if (ctx.enableNMS) {
            float radiusT = sqrt(ctx.imgTemplate.cols * ctx.imgTemplate.cols +
                ctx.imgTemplate.rows * ctx.imgTemplate.rows) * 0.02f;
            float radiusS = sqrt(ctx.imgScene.cols * ctx.imgScene.cols +
                ctx.imgScene.rows * ctx.imgScene.rows) * 0.02f;
            applyNMS(ctx.kpsTemplate, radiusT);
            applyNMS(ctx.kpsScene, radiusS);
        }


        if (ctx.algorithmType == "SIFT") {
            Ptr<SIFT> detector = SIFT::create();
            detector->compute(grayTemplate, ctx.kpsTemplate, ctx.descTemplate);
            detector->compute(grayScene, ctx.kpsScene, ctx.descScene);
        }
        else { // ORB
            Ptr<ORB> detector = ORB::create();
            detector->compute(grayTemplate, ctx.kpsTemplate, ctx.descTemplate);
            detector->compute(grayScene, ctx.kpsScene, ctx.descScene);
        }
        // 结束计时
        auto endTime = chrono::high_resolution_clock::now();
        ctx.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        
        // 分别生成左右图
        generateVisualization(ctx);

        return true;
    }

    string getStepName() override { return "特征点检测"; }


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
        vector<Point2f> corners(4);
        corners[0] = Point2f(0, 0);
        corners[1] = Point2f((float)ctx.imgTemplate.cols, 0);
        corners[2] = Point2f((float)ctx.imgTemplate.cols, (float)ctx.imgTemplate.rows);
        corners[3] = Point2f(0, (float)ctx.imgTemplate.rows);
        float diag1 = norm(corners[0] - corners[2]);  // 左上 → 右下
        float diag2 = norm(corners[1] - corners[3]);  // 右上 → 左下
        float diag = (diag1 + diag2) / 2.0f;  // 平均

        int scale = cvRound(diag * diag) / 2e6;
        scale = max(1, min(10, scale));

        for (auto& kp : ctx.kpsTemplate) {
            kp.size *= scale;
        }
        int scale2 = cvRound(diag * diag) / 2e6;
        scale2 = max(1, min(10, scale));

        for (auto& kp2 : ctx.kpsScene) {
            kp2.size *= scale2;
        }
        drawKeypoints(ctx.imgTemplate, ctx.kpsTemplate, ctx.visLeft, Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        drawKeypoints(ctx.imgScene, ctx.kpsScene, ctx.visRight, Scalar::all(-1),cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    }
};

// ============================================================
// Step：特征匹配
// ============================================================
class Step_MatchFeatures : public IStep {
public:
    bool execute(StitchingContext& ctx) override {
        if (ctx.descTemplate.empty() || ctx.descScene.empty()) return false;

        // 开始计时
        auto startTime = chrono::high_resolution_clock::now();

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
        else { // ORB
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

        // 结束计时
        auto endTime = chrono::high_resolution_clock::now();
        ctx.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();

        ctx.stepImage_Matches = generateVisualization(ctx);
        return true;
    }

    string getStepName() override { return "特征匹配"; }

   

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
// Step：计算单应性矩阵
// ============================================================
class Step_ComputeHomography : public IStep {
public:
    bool execute(StitchingContext& ctx) override {

        // 开始计时
        auto startTime = chrono::high_resolution_clock::now();

        if (ctx.goodMatches.size() < 4) {
            MessageBox(NULL, _T("匹配点不足4个，无法计算单应性矩阵"),
                _T("ImageMatching"), MB_ICONERROR);
            ctx.executionTimeMs = -1;
            return false;
        }
        ctx.visLeft = ctx.imgTemplate;
        ctx.ptsTemplate.clear();
        ctx.ptsScene.clear();
        for (auto& match : ctx.goodMatches) {
            ctx.ptsTemplate.push_back(ctx.kpsTemplate[match.queryIdx].pt);
            ctx.ptsScene.push_back(ctx.kpsScene[match.trainIdx].pt);
        }

        ctx.H = findHomography(ctx.ptsTemplate, ctx.ptsScene, RANSAC, 3.0, ctx.inliersMask);
        if (ctx.H.empty()) {
            MessageBox(NULL, _T("单应性矩阵计算失败"),
                _T("ImageMatching"), MB_ICONERROR);
            ctx.executionTimeMs = -1;
            return false;
        }

        // 结束计时
        auto endTime = chrono::high_resolution_clock::now();
        ctx.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();

        // 根据掩膜挑选出内点 (Inliers)
        for (int i = 0; i < ctx.inliersMask.rows; i++) {
            // mask 中值为 1 表示该点对符合单应性矩阵，即为内点
            if (ctx.inliersMask.at<uchar>(i, 0) == 1) {
                ctx.inlierMatches.push_back(ctx.goodMatches[i]);
            }
        }

        ctx.matchedPointsCount = cv::countNonZero(ctx.inliersMask);
        ctx.totalInitialMatches = (int)ctx.goodMatches.size();
        ctx.accuracyScore = (static_cast<double>(ctx.matchedPointsCount) / ctx.totalInitialMatches) * 100.0;
        return true;
    }

    string getStepName() override { return "单应性矩阵计算"; }
};

// ============================================================
// Step：结果呈现
// ============================================================
class Step_ShowResult : public IStep {
public:
    bool execute(StitchingContext& ctx) override {

        // 开始计时
        auto startTime = chrono::high_resolution_clock::now();
        // 结束计时
        auto endTime = chrono::high_resolution_clock::now();
        ctx.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
        generateVisualization(ctx);
        return true;
    }

    string getStepName() override { return "结果呈现"; }


private:
    Mat generateVisualization(StitchingContext& ctx) {
        Mat vis;

        // 绘制四个角点映射边框
        vector<Point2f> corners(4);
        corners[0] = Point2f(0, 0);
        corners[1] = Point2f((float)ctx.imgTemplate.cols, 0);
        corners[2] = Point2f((float)ctx.imgTemplate.cols, (float)ctx.imgTemplate.rows);
        corners[3] = Point2f(0, (float)ctx.imgTemplate.rows);
        ctx.visRight = ctx.imgScene.clone();
        vector<Point2f> sceneCorners(4);
        perspectiveTransform(corners, sceneCorners, ctx.H);
        // 两点之间的距离 = 对角线长度
        float diag1 = norm(sceneCorners[0] - sceneCorners[2]);  // 左上 → 右下
        float diag2 = norm(sceneCorners[1] - sceneCorners[3]);  // 右上 → 左下
        float diag = (diag1 + diag2) / 2.0f;  // 平均

        int thickness = cvRound(diag * diag) / 3e5;
        thickness = max(2, min(10, thickness));
        for (int i = 0; i < 4; i++) {
            line(ctx.visRight, sceneCorners[i], sceneCorners[(i + 1) % 4], Scalar(0, 255, 0), thickness);
        }

        return ctx.visRight;
    }
};

// ============================================================
// StitchController：流程控制器
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
        m_steps.push_back(make_shared<Step_DetectKeypoints>());
        m_steps.push_back(make_shared<Step_MatchFeatures>());
        m_steps.push_back(make_shared<Step_ComputeHomography>());
        m_steps.push_back(make_shared<Step_ShowResult>());
    }

    void setImages(const Mat& templateImg, const Mat& sceneImg) {
        m_context.imgTemplate = templateImg.clone();
        m_context.imgScene = sceneImg.clone();
        m_context.clearMiddleResults();
        m_currentStep = -1;
        m_context.inlierMatches.clear();
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


    // 获取左右图
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
// 策略类（保留原有接口，兼容旧代码）
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
// 主函数
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
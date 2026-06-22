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
	Scalar color{ 0, 255, 0 }; // 特征点颜色
	Mat descTemplate;
	Mat descScene;

	// 特征匹配
	vector<DMatch> allMatches;
	vector<DMatch> goodMatches;
	vector<DMatch> inlierMatches;

	// 单应性矩阵
	vector<Point2f> ptsTemplate;
	vector<Point2f> ptsScene;
	vector<KeyPoint> goodKeypointsTemplate; // 这两个和上面两个
	vector<KeyPoint> goodKeypointsScene; // 含义相同，仅类型不同
	Mat inliersMask;
	vector<KeyPoint> inlierPtsLeft; // 左图内点
	vector<KeyPoint> inlierPtsRight; // 右图内点
	Mat H;

	// 统计数据
	long executionTimeMs = 0; // 每个步骤更新
	int totalInitialMatches = 0;
	int matchedPointsCount = 0;
	double accuracyScore = 0.0;

	// 步骤可视化图像（分别存放左右图，用于单独显示）
	Mat visLeft;   // 模板图特征/匹配等
	Mat visRight;  // 场景图特征/匹配等

	void clearMiddleResults() {
		kpsTemplate.clear();
		kpsScene.clear();
		descTemplate.release();
		descScene.release();
		allMatches.clear();
		goodMatches.clear();
		ptsTemplate.clear();
		ptsScene.clear();
		goodKeypointsTemplate.clear();
		goodKeypointsScene.clear();
		inliersMask.release();
		H.release();
		visLeft.release();
		visRight.release();
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
	Mat getLeftVisualization(StitchingContext& ctx) { return ctx.visLeft; }
	Mat getRightVisualization(StitchingContext& ctx) { return ctx.visRight; }
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
		drawKeypoints(ctx.imgTemplate, ctx.kpsTemplate, ctx.visLeft, ctx.color, cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		drawKeypoints(ctx.imgScene, ctx.kpsScene, ctx.visRight, ctx.color,cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
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
		
		Ptr<DescriptorMatcher> matcher;

		if (ctx.algorithmType == "SIFT") {
			matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
		}
		else { // ORB
			matcher = DescriptorMatcher::create(DescriptorMatcher::BRUTEFORCE_HAMMING);
		}
		
		vector<vector<DMatch>> knnMatches;
		matcher->knnMatch(ctx.descTemplate, ctx.descScene, knnMatches, 2);

		ctx.allMatches.clear();
		ctx.goodMatches.clear();
		ctx.ptsTemplate.clear();
		ctx.ptsScene.clear();
		ctx.goodKeypointsTemplate.clear();
		ctx.goodKeypointsScene.clear();

		for (auto& km : knnMatches) {
			if (km.size() >= 2 && km[0].distance < 0.75f * km[1].distance) {
				ctx.goodMatches.push_back(km[0]);
				ctx.ptsTemplate.push_back(ctx.kpsTemplate[km[0].queryIdx].pt);
				ctx.ptsScene.push_back(ctx.kpsScene[km[0].trainIdx].pt);
				ctx.goodKeypointsTemplate.push_back(ctx.kpsTemplate[km[0].queryIdx]);
				ctx.goodKeypointsScene.push_back(ctx.kpsScene[km[0].trainIdx]);
			}
		}

		// 结束计时
		auto endTime = chrono::high_resolution_clock::now();
		ctx.executionTimeMs = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();

		drawKeypoints(ctx.imgTemplate, ctx.goodKeypointsTemplate, ctx.visLeft, ctx.color, cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		drawKeypoints(ctx.imgScene, ctx.goodKeypointsScene, ctx.visRight, ctx.color, cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

		return true;
	}

	string getStepName() override { return "特征匹配"; }
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

		// 根据掩膜挑选出内点
		for (int i = 0; i < ctx.inliersMask.rows; i++) {
			// mask 中值为 1 表示该点对符合单应性矩阵，即为内点
			if (ctx.inliersMask.at<uchar>(i, 0) == 1) {
				DMatch match = ctx.goodMatches[i];
				ctx.inlierMatches.push_back(match);
				ctx.inlierPtsLeft.push_back(ctx.goodKeypointsTemplate[i]);
				ctx.inlierPtsRight.push_back(ctx.goodKeypointsScene[i]);
			}
		}

		ctx.matchedPointsCount = cv::countNonZero(ctx.inliersMask);
		ctx.totalInitialMatches = (int)ctx.goodMatches.size();
		ctx.accuracyScore = (static_cast<double>(ctx.matchedPointsCount) / ctx.totalInitialMatches) * 100.0;
		
		drawKeypoints(ctx.imgTemplate, ctx.inlierPtsLeft, ctx.visLeft, ctx.color, cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		drawKeypoints(ctx.imgScene, ctx.inlierPtsRight, ctx.visRight, ctx.color, cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		
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
		ctx.visLeft = ctx.imgTemplate;
		vector<Point2f> sceneCorners(4);
		perspectiveTransform(corners, sceneCorners, ctx.H);
		// 两点之间的距离 = 对角线长度
		float diag1 = norm(sceneCorners[0] - sceneCorners[2]);  // 左上 → 右下
		float diag2 = norm(sceneCorners[1] - sceneCorners[3]);  // 右上 → 左下
		float diag = (diag1 + diag2) / 2.0f;  // 平均

		int thickness = cvRound(diag * diag) / 3e5;
		thickness = max(2, min(10, thickness));
		for (int i = 0; i < 4; i++) {
			line(ctx.visRight, sceneCorners[i], sceneCorners[(i + 1) % 4], ctx.color, thickness);
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
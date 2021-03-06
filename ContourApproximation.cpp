#include "stdafx.h"
#include "common.h"
#include<random>
#include <stack	> 
#include <iostream>
#include <fstream>
#include<algorithm>
#include <numeric>
#include <stdlib.h>
#define ghettoAtY(pointer,begin,cols)  ((pointer- begin) / cols)
#define ghettoAtX(pointer,begin,cols)  ((pointer - begin) % cols)
vector<char> getChain(Mat src,const uchar** start) {
		int cols = src.cols;
		std::vector<char> dst;

		uchar dir = 7;
		cv::MatIterator_<uchar> b = src.begin<uchar>();
		cv::MatIterator_<uchar> e = src.end<uchar>();
		cv::MatIterator_<uchar> first = std::find_if(b, e, [](uchar& x) {return x < 200; });
		*start = first.ptr;
		MatIterator_<uchar> second = detectBorderStep(&dir, first, cols);
		dst.push_back(dir);
		MatIterator_<uchar> previous = detectBorderStep(&dir, second, cols);
		dst.push_back(dir);
		MatIterator_<uchar> current = detectBorderStep(&dir, previous, cols);
		while (first != previous || second != current) {
			dst.push_back(dir);
			previous = current;
			current = detectBorderStep(&dir, current, cols);
		}
		dst.pop_back();
		//+- one pushback needed;
		return dst;
}
vector<const uchar*> getBorder(vector<char> chain,Mat src,const uchar* first) {
	int cols = src.cols;
	int rows = src.rows;
	const uchar* current = first;
	vector<const uchar*> returnBox;
	returnBox.push_back(first);
	for_each(chain.begin(), chain.end(), [&](char x) {
		switch (x) {
		case 0: current = current + 1; break;
		case 1: current = current - cols + 1; break;
		case 2: current = current - cols; break;
		case 3: current = current - cols - 1; break;
		case 4: current = current - 1; break;
		case 5: current = current + cols - 1; break;
		case 6: current = current + cols; break;
		case 7: current = current + cols + 1; break;
		}
		returnBox.push_back(current);
		});
	return returnBox;
}
float linePointDist(float x0, float x1, float x2, float y0, float y1, float y2) {
	return (std::abs((y2-y1)*x0 - (x2 - x1)*y0 + x2*y1 - y2*x1)) / (sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}
float pointPointDist(float x1, float x2, float y1, float y2) {
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
void segment(int indexP1, int indexP2,Mat src, vector<const uchar*> border,int threshold,vector<int>* result) {
	float dist = 0;
	int cols = src.cols;
	const uchar* beginPtr = src.begin<uchar>().ptr;
	const uchar* point1 = border[indexP1];
	const uchar* point2 = border[indexP2];
	float xP1 = ghettoAtX(point1, beginPtr, cols);
	float yP1 = ghettoAtY(point1, beginPtr, cols);
	float xP2 = ghettoAtX(point2, beginPtr, cols);
	float yP2 = ghettoAtY(point2, beginPtr, cols);
	float maxDist = 0;
	int index = indexP1 - 1;
	int indexMax = 0;
	int endPtrValue = border.size() - indexP2;
	endPtrValue++;
	for_each(border.begin() + indexP1, border.end()- endPtrValue, [&](const uchar* a) {
		index++;
		float xA = ghettoAtX(a, beginPtr, cols);
		float yA = ghettoAtY(a, beginPtr, cols);
		float dist = linePointDist(xA, xP1, xP2, yA, yP1, yP2);
		if (dist > maxDist) {
			maxDist = dist;
			indexMax = index;
		}
		});
	if (maxDist > threshold) {
		segment(indexP1, indexMax, src, border, threshold, result);
		segment(indexMax, indexP2, src, border, threshold, result);
	}
	else {
		result->push_back(indexP1);
	}
}
void proj() {
	char fname[MAX_PATH];
	while (openFileDlg(fname)) {
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		Mat dst = Mat(src.rows, src.cols, CV_8UC1,255);
		int threshold;
		printf("input the threshold:\n");
		scanf("%d", &threshold);
		int cols = src.cols;
		//Mat dst = Mat(src.rows, src.cols, CV_8UC1, 255);
		const uchar* start;
		vector<char> chain = getChain(src, &start);
		vector<const uchar*> border = getBorder(chain,src,start);
		const uchar* beginPtr = src.begin<uchar>().ptr;
		MatIterator_<uchar> srcPtr = src.begin<uchar>();
		MatIterator_<uchar> srcPtrFarthest = src.begin<uchar>();
		float xBegin = ghettoAtX(border[0], beginPtr, cols);
		float yBegin = ghettoAtY(border[0], beginPtr, cols);
		float largestDist = 0;
		int index=-1;
		int maxIndex;
		for_each(border.begin(), border.end(), [&](const uchar* a) {
			index++;
			float xA = ghettoAtX(a, beginPtr, cols);
			float yA = ghettoAtY(a, beginPtr, cols);
			float distA = pointPointDist(xBegin,xA,yBegin,yA);
			if (distA > largestDist) {
				largestDist = distA;
				maxIndex = index;
			}
			});
		vector<int> result;
		segment(0, maxIndex,src,border,threshold,&result);
		segment(maxIndex,border.size()-1, src, border, threshold, &result);
		result.push_back(border.size() - 1);
		int firstI = result[0];
		for_each(result.begin()+1, result.end(), [&](int i) {
			const uchar* point1 = border[firstI];
			const uchar* point2 = border[i];
			int xFI = (int)ghettoAtX(point1, beginPtr, cols);
			int yFI = (int)ghettoAtY(point1, beginPtr, cols);
			int xI = (int)ghettoAtX(point2, beginPtr, cols);
			int yI = (int)ghettoAtY(point2, beginPtr, cols);
			cv::line(dst, Point(xFI, yFI), Point(xI, yI), 0);
			firstI = i;
			});
		//found the far pixel(i gues idk, must re-check formula)
		//recursively split
		imshow("test", dst);

	}
}

/*
 * Copyright 2019 Xilinx Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * NOTICE: This file has been modified from the original verson.
 *  The original code for drawing bounding boxes was taken from
 *  https://github.com/Xilinx/Vitis-AI/blob/v1.1/Vitis-AI-Library/ssd/test/test_ssd.cpp
 *
 *  TJS - Removed floating-point round operations; that level of accuracy is
 *        not needed for demonstration purposes
 *      - Created DrawBoxes template function 
 *      - Removed other code that wasn't used in this app context
 */

#include <iostream>
#include <opencv2/opencv.hpp>

template<class T>
void DrawBoxes( cv::Mat img, T results, cv::Scalar color = cv::Scalar(0, 255, 0))
{
  /* Draw bounding boxes */
  for (auto &box : results)
  {
    int xmin = box.x * img.cols;
    int ymin = box.y * img.rows;
    int xmax = xmin + (box.width * img.cols);
    int ymax = ymin + (box.height * img.rows);

    xmin = std::min(std::max(xmin, 0), img.cols);
    xmax = std::min(std::max(xmax, 0), img.cols);
    ymin = std::min(std::max(ymin, 0), img.rows);
    ymax = std::min(std::max(ymax, 0), img.rows);

    cv::rectangle(img, cv::Point(xmin, ymin), cv::Point(xmax, ymax), color, 2, 1, 0);
  }
}



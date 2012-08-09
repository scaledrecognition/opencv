/*M///////////////////////////////////////////////////////////////////////////////////////
//
// IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
// By downloading, copying, installing or using the software you agree to this license.
// If you do not agree to this license, do not download, install,
// copy or use the software.
//
//
//                          License Agreement
//               For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2008-2011, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
//  * The name of the copyright holders may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//M*/

#include "precomp.hpp"
#include <string>
#include <iostream>

#ifdef HAVE_CUDA

namespace {

    struct GreedyLabeling
    {
        struct dot
        {
            int x;
            int y;

            static dot make(int i, int j)
            {
                dot d; d.x = i; d.y = j;
                return d;
            }
        };

        struct InInterval
        {
            InInterval(const int& _lo, const int& _hi) : lo(-_lo), hi(_hi) {};
            const int lo, hi;

            bool operator() (const unsigned char a, const unsigned char b) const
            {
                int d = a - b;
                return lo <= d && d <= hi;
            }
        };

        GreedyLabeling(cv::Mat img)
        : image(img), _labels(image.size(), CV_32SC1, cv::Scalar::all(-1)) {}

        void operator() (cv::Mat labels) const
        {
            InInterval inInt(0, 2);
            dot* stack = new dot[image.cols * image.rows];

            int cc = -1;

            int* dist_labels = (int*)labels.data;
            int pitch = labels.step1();

            unsigned char* source = (unsigned char*)image.data;
            int width = image.cols;
            int height = image.rows;

            for (int j = 0; j < image.rows; ++j)
                for (int i = 0; i < image.cols; ++i)
                {
                    if (dist_labels[j * pitch + i] != -1) continue;

                    dot* top = stack;
                    dot p = dot::make(i, j);
                    cc++;

                    dist_labels[j * pitch + i] = cc;

                    while (top >= stack)
                    {
                        int*  dl = &dist_labels[p.y * pitch + p.x];
                        unsigned char* sp = &source[p.y * image.step1() + p.x];

                        dl[0] = cc;

                        //right
                        if( p.x < (width - 1) && dl[ +1] == -1 && inInt(sp[0], sp[+1]))
                            *top++ = dot::make(p.x + 1, p.y);

                        //left
                        if( p.x > 0 && dl[-1] == -1 && inInt(sp[0], sp[-1]))
                            *top++ = dot::make(p.x - 1, p.y);

                        //bottom
                        if( p.y < (height - 1) && dl[+pitch] == -1 && inInt(sp[0], sp[+image.step1()]))
                            *top++ = dot::make(p.x, p.y + 1);

                        //top
                        if( p.y > 0 && dl[-pitch] == -1 && inInt(sp[0], sp[-image.step1()]))
                            *top++ = dot::make(p.x, p.y - 1);

                        p = *--top;
                    }
                }
            delete[] stack;
        }

        void checkCorrectness(cv::Mat gpu)
        {
            cv::Mat diff = gpu - _labels;

            int outliers = 0;
            for (int j = 0; j < image.rows; ++j)
                for (int i = 0; i < image.cols; ++i)
                {
                    if ( (_labels.at<int>(j,i) == gpu.at<int>(j,i + 1)) && (diff.at<int>(j, i) != diff.at<int>(j,i + 1)))
                    {
                        outliers++;
                        // std::cout <<  j << " " << i << " " << _labels.at<int>(j,i) << " " << gpu.at<int>(j,i + 1) << " " << diff.at<int>(j, i) << " " << diff.at<int>(j,i + 1) << std::endl;
                    }
                }
            ASSERT_FALSE(outliers);
        }

        cv::Mat image;
        cv::Mat _labels;
    };
}

struct Labeling : testing::TestWithParam<cv::gpu::DeviceInfo>
{
    cv::gpu::DeviceInfo devInfo;

    virtual void SetUp()
    {
        devInfo = GetParam();
        cv::gpu::setDevice(devInfo.deviceID());
    }

    cv::Mat loat_image()
    {
        return cv::imread(std::string( cvtest::TS::ptr()->get_data_path() ) + "labeling/IMG_0727.JPG");
    }
};

TEST_P(Labeling, ConnectedComponents)
{
    cv::Mat image;
    cvtColor(loat_image(), image, CV_BGR2GRAY);

    cv::threshold(image, image, 150, 255, CV_THRESH_BINARY);

    ASSERT_TRUE(image.type() == CV_8UC1);

    GreedyLabeling host(image);
    host(host._labels);

    cv::gpu::GpuMat mask;
    mask.create(image.rows, image.cols, CV_8UC1);

    cv::gpu::GpuMat components;
    components.create(image.rows, image.cols, CV_32SC1);

    cv::gpu::connectivityMask(cv::gpu::GpuMat(image), mask, cv::Scalar::all(0), cv::Scalar::all(2));

    ASSERT_NO_THROW(cv::gpu::labelComponents(mask, components));

    host.checkCorrectness(cv::Mat(components));
    cv::imshow("test", image);
    cv::waitKey(0);
    cv::imshow("test", host._labels);
    cv::waitKey(0);
}

INSTANTIATE_TEST_CASE_P(ConnectedComponents, Labeling, ALL_DEVICES);

#endif
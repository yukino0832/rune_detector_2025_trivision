#pragma once

#include "Detector.hpp"
#include "Param.hpp"
#include "Utility.hpp"

const std::string CONFIG_PATH = "../config.yaml";

namespace rune
{
    class Rune_detect
    {
    public:
        Rune_detect();
        bool runOnce(const cv::Mat &image, double pitch, double yaw, double roll = 0.0);

    private:
        Param m_param;
        Detector m_detector;
    };

} // namespace power_rune
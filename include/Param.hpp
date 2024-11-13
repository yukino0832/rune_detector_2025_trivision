#pragma once
#include "Utility.hpp"

namespace rune
{
    /**
     * 0: 不显示
     * 1: 显示箭头、装甲板、中心、预测点
     * 2: 在 1 的基础上显示灯条、roi
     * 3: 在 2 的基础上显示二值化图片
     */
#define SHOW_IMAGE 3

#define CONSOLE_OUTPUT 3

    struct Param
    {
        Param() = default;
        Param(const std::string &filename);

        void load(const std::string &filename);

        // 图像
        inline static float IMAGE_WIDTH, IMAGE_HEIGHT;

        // 颜色
        const inline static cv::Scalar RED{0, 0, 255};
        const inline static cv::Scalar BLUE{255, 0, 0};
        const inline static cv::Scalar GREEN{0, 255, 0};
        const inline static cv::Scalar WHITE{255, 255, 255};
        const inline static cv::Scalar YELLOW{0, 255, 255};
        const inline static cv::Scalar PURPLE{128, 0, 128};

        // 帧率
        inline static int FPS;

        inline static cv::Scalar DRAW_COLOR;

        // 装甲板
        inline static double ARMOR_RADIUS;

        // 符半径
        inline static const double POWER_RUNE_RADIUS{700.0};

        // 大小符
        inline static Mode MODE;

        inline static Color COLOR;

        // 亮度阈值
        inline static int ARROW_BRIGHTNESS_THRESHOLD;
        inline static int ARMOR_BRIGHTNESS_THRESHOLD;
        inline static const int MAX_BRIGHTNESS{255};

        // ROI参数
        inline static double LOCAL_ROI_DISTANCE_RATIO;
        inline static float LOCAL_ROI_WIDTH;

        inline static float ARMOR_CENTER_VERTICAL_DISTANCE_THRESHOLD;

        inline static double GLOBAL_ROI_LENGTH_RATIO;

        // 构成箭头有效检测的阈值
        inline static double MIN_ARROW_LIGHTLINE_AREA;
        inline static double MAX_ARROW_LIGHTLINE_AREA;

        inline static double MAX_ARROW_LIGHTLINE_ASPECT_RATIO;

        inline static int MIN_ARROW_LIGHTLINE_NUM;
        inline static int MAX_ARROW_LIGHTLINE_NUM;

        inline static double MIN_ARROW_ASPECT_RATIO;
        inline static double MAX_ARROW_ASPECT_RATIO;

        inline static double MAX_ARROW_AREA;

        inline static double MAX_SAME_ARROW_AREA_RATIO;

        // 构成装甲板有效检测的阈值
        inline static double MIN_ARMOR_CIRCLELIGHT_RADIUS;
        inline static double MAX_ARMOR_CIRCLELIGHT_RADIUS;

        inline static double MIN_ARMOR_CIRCLELIGHT_AREA;
        inline static double MAX_ARMOR_CIRCLELIGHT_AREA;

        // R标所在区域
        inline static double MIN_CENTER_AREA;
        inline static double MAX_CENTER_AREA;

        inline static double MAX_CENTER_ASPECT_RATIO;

        inline static int MIN_FIT_DATA_SIZE;
        inline static int MAX_FIT_DATA_SIZE;
    };
} // namespace rune
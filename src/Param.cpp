#include "../include/Param.hpp"

namespace rune
{
    Param::Param(const std::string &filename)
    {
        load(filename);
    }

    void Param::load(const std::string &filename)
    {
        cv::FileStorage fs(filename, cv::FileStorage::READ);

        // color
        std::string colorStr;
        fs["color"] >> colorStr;
        if (std::transform(colorStr.begin(), colorStr.end(), colorStr.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            colorStr == "red")
        {
            COLOR = Color::RED;
        }
        else if (colorStr == "blue")
        {
            COLOR = Color::BLUE;
        }
        else
        {
            throw std::runtime_error("unknown color " + colorStr);
        }
        DRAW_COLOR = COLOR == Color::BLUE ? RED : BLUE;

        // fps
        fs["fps"] >> FPS;

        // image width and height
        auto fsImage = fs["image"];
        fsImage["width"] >> IMAGE_WIDTH;
        fsImage["height"] >> IMAGE_HEIGHT;

        // brightness threshold
        auto fsDetect = fs["detect"];
        auto fsBrightness = fsDetect["brightness_threshold"][colorStr];
        fsBrightness["arrow"] >> ARROW_BRIGHTNESS_THRESHOLD;
        fsBrightness["armor"] >> ARMOR_BRIGHTNESS_THRESHOLD;

        // local roi
        auto fsLocalRoi = fsDetect["local_roi"];
        fsLocalRoi["distance_ratio"] >> LOCAL_ROI_DISTANCE_RATIO;
        fsLocalRoi["width"] >> LOCAL_ROI_WIDTH;

        // vertical distance thresh from armor to centerR
        fsDetect["armor_center_vertical_distance_threshold"] >> ARMOR_CENTER_VERTICAL_DISTANCE_THRESHOLD;

        // global roi length ratio
        fsDetect["global_roi_length_ratio"] >> GLOBAL_ROI_LENGTH_RATIO;

        // arrow
        auto fsArrow = fsDetect["arrow"];
        fsArrow["lightline"]["area"]["min"] >> MIN_ARROW_LIGHTLINE_AREA;
        fsArrow["lightline"]["area"]["max"] >> MAX_ARROW_LIGHTLINE_AREA;
        fsArrow["lightline"]["aspect_ratio_max"] >> MAX_ARROW_LIGHTLINE_ASPECT_RATIO;
        fsArrow["lightline"]["num"]["min"] >> MIN_ARROW_LIGHTLINE_NUM;
        fsArrow["lightline"]["num"]["max"] >> MAX_ARROW_LIGHTLINE_NUM;
        fsArrow["same_area_ratio_max"] >> MAX_SAME_ARROW_AREA_RATIO;
        fsArrow["aspect_ratio"]["min"] >> MIN_ARROW_ASPECT_RATIO;
        fsArrow["aspect_ratio"]["max"] >> MAX_ARROW_ASPECT_RATIO;
        fsArrow["area_max"] >> MAX_ARROW_AREA;

        // armor
        /*
         * TODO
         */

        // centerR
        auto fsCenterR = fsDetect["centerR"];
        fsCenterR["area"]["min"] >> MIN_CENTER_AREA;
        fsCenterR["area"]["max"] >> MAX_CENTER_AREA;
        fsCenterR["aspect_ratio_max"] >> MAX_CENTER_ASPECT_RATIO;

        // mode
        std::string modeStr;
        fs["mode"] >> modeStr;
        if (std::transform(modeStr.begin(), modeStr.end(), modeStr.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            modeStr == "small")
        {
            MODE = Mode::SMALL;
        }
        else if (modeStr == "big")
        {
            MODE = Mode::BIG;
        }
        else
        {
            throw std::runtime_error("unknown mode " + modeStr);
        }
    }

} // namespace rune
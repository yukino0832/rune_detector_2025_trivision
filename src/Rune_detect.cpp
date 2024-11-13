#include "../include/Rune_detect.hpp"

#include <thread>

namespace rune
{
    extern std::mutex MUTEX;

    Rune_detect::Rune_detect() : m_param{CONFIG_PATH} {}

    bool Rune_detect::runOnce(const cv::Mat &image, double pitch, double yaw, double roll)
    {
        Frame frame{image, std::chrono::steady_clock::now(), pitch, yaw, roll};

        if (m_detector.detect(frame) == false)
        {
            return false;
        }

        m_detector.visualize();

        char key = cv::waitKey(1);
        if (key == ' ')
        {
            cv::waitKey(0);
        }
        else if (key == 'q')
        {
            std::exit(1);
        }

        return true;
    }
} // namespace rune
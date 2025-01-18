#include <thread>
#include <fstream>

#include "include/MindvisionCameras.hpp"

#include "include/Rune_detect.hpp"

const std::string VIDEO_PATH = "../video/example.mp4";

#define PERSPECTIVE_IMAGE 0
#define USE_MANDVISION 0

int main()
{
    rune::Rune_detect pr;
    cv::VideoCapture cap{VIDEO_PATH};
    // MindvisionCameras camera;
    cv::Mat image;
    while (true)
    {
        auto start{std::chrono::steady_clock::now()};
        if (USE_MANDVISION == 1)
        {
            // image = camera.CameraGetImage("041062620105");
            // image = camera.CameraGetImage("042003320465");
        }
        else
        {
            if (cap.read(image) == false)
            {

                break;
            }
        }

        pr.runOnce(image, 0.0, 0.0);

        auto end{std::chrono::steady_clock::now()};
        auto process_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Processing time: " << process_time.count() << " ms" << std::endl;

        auto future_time = start + std::chrono::milliseconds(1000 / rune::Param::FPS);
        if (std::chrono::steady_clock::now() < future_time)
        {
            std::this_thread::sleep_until(future_time);
        }
    }

    return 0;
}
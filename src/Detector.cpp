#include "../include/Detector.hpp"

namespace rune
{
    std::mutex MUTEX;

    double Arrow_angle;

    /**
     * @brief Construct a new Lightline:: Lightline object
     * @param[in] contour       轮廓点集
     * @param[in] roi           roi 用来设置正确的中心及角点
     */
    Lightline::Lightline(const std::vector<cv::Point> &contour, const cv::Rect2f &localRoi,
                         const cv::Rect2f &globalRoi)
        : m_contour(contour), m_contourArea(cv::contourArea(contour)), m_rotatedRect(cv::minAreaRect(contour))
    {
        // 长的为 length，短的为 width
        m_width = m_rotatedRect.size.width, m_length = m_rotatedRect.size.height;
        if (m_width > m_length)
        {
            std::swap(m_width, m_length);
        }
        m_aspectRatio = m_length / m_width;
        m_center = m_rotatedRect.center;
        m_angle = m_rotatedRect.angle;
        m_area = m_rotatedRect.size.width * m_rotatedRect.size.height;
        std::array<cv::Point2f, 4> points;
        m_rotatedRect.points(points.begin());
        /**
         * OpenCV 中 RotatedRect::points() 角点顺序为顺时针，p[0]
         * 为纵坐标最大的点。若有多个纵坐标最大，则取其中横坐标最大的点。 p[0] 到 p[3] 的边为 width，其邻边为
         * height。 根据上述关系可以确立四个角点位置。如果是装甲板灯条，则其还需要结合中心 R 来得到中心 R
         * 参照下的角点位置。
         */
        if (m_rotatedRect.size.width > m_rotatedRect.size.height)
        {
            m_tl = points[1];
            m_tr = points[2];
            m_bl = points[0];
            m_br = points[3];
        }
        else
        {
            m_tl = points[0];
            m_tr = points[1];
            m_bl = points[3];
            m_br = points[2];
        }
        // 得到相对原图的角点和中心位置
        m_tl += localRoi.tl() + globalRoi.tl();
        m_tr += localRoi.tl() + globalRoi.tl();
        m_bl += localRoi.tl() + globalRoi.tl();
        m_br += localRoi.tl() + globalRoi.tl();
        m_center += localRoi.tl() + globalRoi.tl();
        m_x = m_center.x, m_y = m_center.y;
    }

    /**
     * @brief Construct a new Lightline:: Circlelight object
     * @param[in] contour       轮廓点集
     * @param[in] roi           roi 用来设置正确的中心及角点
     */
    Circlelight::Circlelight(const cv::Vec3f &circle, const cv::Rect2f &localRoi,
                             const cv::Rect2f &globalRoi)
    {
        m_center = cv::Point2f(circle[0], circle[1]);
        m_radius = circle[2];
        int numPoints = 100; // 用于近似圆的点数
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = 2 * CV_PI * i / numPoints;
            float x = m_center.x + m_radius * std::cos(angle);
            float y = m_center.y + m_radius * std::sin(angle);
            m_contour.emplace_back(x, y);
        }
        m_rotatedRect = cv::minAreaRect(m_contour);
        m_rotatedRect.angle = Arrow_angle;
        m_area = m_rotatedRect.size.width * m_rotatedRect.size.height;
        m_aspectRatio = (m_rotatedRect.size.width > m_rotatedRect.size.height) ? m_rotatedRect.size.width / m_rotatedRect.size.height : m_rotatedRect.size.height / m_rotatedRect.size.width;
        std::array<cv::Point2f, 4> points;
        m_rotatedRect.points(points.begin());
        if (m_rotatedRect.size.width > m_rotatedRect.size.height)
        {
            m_tl = points[1];
            m_tr = points[2];
            m_bl = points[0];
            m_br = points[3];
        }
        else
        {
            m_tl = points[0];
            m_tr = points[1];
            m_bl = points[3];
            m_br = points[2];
        }
        // 得到相对原图的角点和中心位置
        m_tl += localRoi.tl() + globalRoi.tl();
        m_tr += localRoi.tl() + globalRoi.tl();
        m_bl += localRoi.tl() + globalRoi.tl();
        m_br += localRoi.tl() + globalRoi.tl();
        m_center += localRoi.tl() + globalRoi.tl();
        m_x = m_center.x, m_y = m_center.y;
    }

    /**
     * @brief 设置箭头
     * @param[in] points 点集
     * @param[in] roi
     */
    void Arrow::set(const std::vector<Lightline> &lightlines, const cv::Point2f &roi)
    {
        std::vector<cv::Point2f> arrowPoints;
        double fillArea = 0.0;
        double pointLineThresh = 0.0;

        /**
         * 将Lightline对象的轮廓点添加到arrowPoints向量的末尾
         * 累加Lightline对象的轮廓面积到fillArea
         * 累加Lightline对象的长度除以lightlines的大小到pointLineThresh
         */
        std::for_each(lightlines.begin(), lightlines.end(), [&](const Lightline &l)
                      {
        arrowPoints.insert(arrowPoints.end(), l.m_contour.begin(), l.m_contour.end());
        fillArea += l.m_contourArea;
        pointLineThresh += l.m_length / lightlines.size(); });

        // 滤除距离较大的点
        m_contour.clear();
        cv::Vec4f line;
        cv::fitLine(arrowPoints, line, cv::DIST_L2, 0, 0.01, 0.01);
        for (const auto &point : arrowPoints)
        {
            if (pointLineDistance(point, line) < pointLineThresh)
            {
                m_contour.push_back(point);
            }
        }

        // 设置成员变量
        m_rotatedRect = cv::minAreaRect(m_contour);
        m_center = m_rotatedRect.center + roi;
        m_length = m_rotatedRect.size.height;
        m_width = m_rotatedRect.size.width;
        // RotatedRect::angle 范围为 -90~0. 这里根据长宽长度关系，将角度扩展到 -90~90
        if (m_length < m_width)
        {
            m_angle = m_rotatedRect.angle;
            // 长的为 length
            std::swap(m_length, m_width);
        }
        else
        {
            m_angle = m_rotatedRect.angle + 90;
        }
        m_aspectRatio = m_length / m_width;
        m_area = m_length * m_width;
        m_fillRatio = fillArea / m_area;
        return;
    }

    /**
     * @brief 寻找符合箭头要求的灯条，并将其存入一个向量中。成功返回 true，否则返回 false
     * @param[in] binary        二值图
     * @param[in] lightlines    输出的灯条向量
     * @param[in] roi           roi，用来设置灯条的正确位置
     * @return true
     * @return false
     */
    void findArrowLightlines(const cv::Mat &binary, std::vector<Lightline> &lightlines, const cv::Rect2f &roi)
    {
        // 寻找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
        for (const auto &contour : contours)
        {
            Lightline lightline(contour, roi);
            // 判断面积
            if (inRange(lightline.m_area, Param::MIN_ARROW_LIGHTLINE_AREA, Param::MAX_ARROW_LIGHTLINE_AREA) ==
                false)
            {
                continue;
            }
            // 判断长宽比
            if (lightline.m_aspectRatio > Param::MAX_ARROW_LIGHTLINE_ASPECT_RATIO)
            {
                continue;
            }
            // 符合要求，则存入
            lightlines.emplace_back(std::move(lightline));
        }
    }

    /**
     * @brief 根据提取的灯条匹配箭头，成功返回 true，否则返回 false
     * @param[in] lightlines    灯条向量
     * @param[in] arrowPtr      指向箭头的指针
     * @param[in] roi           roi，用来设置箭头的正确位置
     * @return true
     * @return false
     */
    bool findArrow(Arrow &arrow, const std::vector<Lightline> &lightlines, const cv::Rect2f &roi)
    {
        // 利用 cv::partition 匹配箭头
        std::vector<int> labels;
        cv::partition(lightlines, labels, sameArrow);
        // data 记录了标识号和其对应次数
        std::vector<std::pair<int, int>> data;
        for (auto label : labels)
        {
            // 对每个 label，从已记录的数据中寻找是否有这个条目，有则对应计数项 +1，否则新增一个条目
            auto iter = std::find_if(data.begin(), data.end(),
                                     [label](const std::pair<int, int> &unit)
                                     { return unit.first == label; });
            if (iter == data.end())
            {
                data.emplace_back(label, 1);
            }
            else
            {
                iter->second += 1;
            }
        }
        if (data.empty() == true)
        {
            return false;
        }
        // 寻找出现次数最多的 label 和其对应的 num
        auto [maxLabel, maxNum]{*std::max_element(
            data.begin(), data.end(),
            [](const std::pair<int, int> &i, const std::pair<int, int> &j)
            { return i.second < j.second; })};
        // 判断 num 是否符合要求
        if (inRange(maxNum, Param::MIN_ARROW_LIGHTLINE_NUM, Param::MAX_ARROW_LIGHTLINE_NUM) == false)
        {
            return false;
        }
        // 再次遍历 labels，选取和 maxLabel 相同的 label，并存入一个向量
        std::vector<int> arrowIndices;
        for (unsigned int i = 0; i < labels.size(); ++i)
        {
            if (labels[i] == maxLabel)
            {
                arrowIndices.push_back(i);
            }
        }
        // 根据这个向量，将其对应的灯条轮廓点集中每个点存入箭头点的向量中
        std::vector<Lightline> arrowLightlines;
        for (auto index : arrowIndices)
        {
            arrowLightlines.push_back(lightlines.at(index));
        }
        // 设置这个箭头
        arrow.set(arrowLightlines, roi.tl());
        // 判断长宽比
        if (inRange(arrow.m_aspectRatio, Param::MIN_ARROW_ASPECT_RATIO, Param::MAX_ARROW_ASPECT_RATIO) == false)
        {
            return false;
        }
        // 判断面积
        if (arrow.m_area > Param::MAX_ARROW_AREA)
        {
            return false;
        }
        Arrow_angle = arrow.m_angle;
        return true;
    }

    /**
     * @brief 比较两个灯条是否满足在一个箭头内的条件，是则返回 true，否则为 false
     * @param[in] l1
     * @param[in] l2
     * @return true
     * @return false
     */
    bool sameArrow(const Lightline &l1, const Lightline &l2)
    {
        // 判断面积比
        double areaRatio{l1.m_area / l2.m_area};
        if (inRange(areaRatio, 1 / Param::MAX_SAME_ARROW_AREA_RATIO, Param::MAX_SAME_ARROW_AREA_RATIO) == false)
        {
            return false;
        }
        // 判断距离
        double distance{pointPointDistance(l1.m_rotatedRect.center, l2.m_rotatedRect.center)};
        double maxDistance{1.2 * (l1.m_width + l2.m_width)};
        if (distance > maxDistance)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief 设置装甲板参数
     * @param[in] circlelight   圆形灯条
     */
    void Armor::set(const Circlelight &circlelight)
    {
        m_center = circlelight.m_center;
        m_x = m_center.x, m_y = m_center.y;
        m_circlelight = circlelight;
        m_tl = circlelight.m_tl;
        m_tr = circlelight.m_tr;
        m_bl = circlelight.m_bl;
        m_br = circlelight.m_br;
        return;
    }

    /**
     * @brief
     * 设置装甲板角点，注意大小为4，顺序必须为装甲板左下角点、左上角点、右上角点、右下角点
     * @param[in] points        输入的角点向量
     */
    void Armor::setCornerPoints(const std::vector<cv::Point2f> &points)
    {
        m_bl = points.at(0);
        m_tl = points.at(1);
        m_tr = points.at(2);
        m_br = points.at(3);
    }

    /**
     * @brief 寻找装甲板圆形灯条，并将其存入一个Circlelight对象。成功返回 true，否则返回
     * false。
     * @param[in] image         带有中心区域的图片
     * @param[in] Circlelight   存储的圆形灯条
     * @param[in] globalRoi     全局 roi
     * @param[in] localRoi      局部 roi
     * @return true
     * @return false
     */
    bool findArmorCirclelight(const cv::Mat &image, Circlelight &circlelight,
                              const cv::Rect2f &globalRoi, const cv::Rect2f &localRoi)
    {
        // 霍夫圆检测
        std::vector<cv::Vec3f> circles;
        cv::GaussianBlur(image, image, cv::Size(9, 9), 2, 2);
        cv::HoughCircles(image, circles, cv::HOUGH_GRADIENT, 1, 0.01, 100, 60, Param::MIN_ARMOR_CIRCLELIGHT_RADIUS, Param::MAX_ARMOR_CIRCLELIGHT_RADIUS);
        if (circles.empty())
        {
            return false; // 没有检测到圆
        }
        // 更新半径最大的圆
        cv::Vec3f max_circle = *std::max_element(circles.begin(), circles.end(),
                                                 [](const cv::Vec3f &a, const cv::Vec3f &b)
                                                 {
                                                     return a[2] < b[2];
                                                 });
        circlelight = Circlelight(max_circle, globalRoi, localRoi);
        // cv::circle(image, cv::Point2f(max_circle[0], max_circle[1]), max_circle[2], cv::Scalar(0, 255, 0), 2);
        // cv::imshow("circle", image);
        // cv::waitKey(0);
        return true;
    }

    /**
     * @brief 根据提取的灯条匹配装甲板，成功返回 true，否则返回 false
     * @param[in] circlelight   圆形灯条
     * @param[in] centers       中心灯条
     * @param[in] armorPtr      装甲板
     * @param[in] arrowPtr      箭头
     * @return true
     * @return false
     */
    bool findArmor(Armor &armor, const Circlelight &circlelight_possible, const Arrow &arrow)
    {
        if (inRange(circlelight_possible.m_area, Param::MIN_ARMOR_CIRCLELIGHT_AREA, Param::MAX_ARMOR_CIRCLELIGHT_AREA) == false)
        {
            return false;
        }
        // 此处判断装甲板中心与箭头中心的距离，如果不符则检测失败
        if (inRange(pointPointDistance(circlelight_possible.m_center, arrow.m_center), arrow.m_length * 0.8, arrow.m_length * 1.5) == false)
        {
            return false;
        }
        armor.set(circlelight_possible);
        return true;
    }

    /**
     * @brief 设置中心 R
     * @param[in] lightline
     */
    void CenterR::set(const Lightline &lightline)
    {
        m_lightline = lightline;
        m_boundingRect = cv::boundingRect(lightline.m_contour);
        // 由于灯条角点和中心点已经设置过 roi，因此这里不需要重新设置
        m_center = lightline.m_center;
        m_x = m_center.x, m_y = m_center.y;
        return;
    }

    /**
     * @brief 寻找符合中心 R 要求的中心灯条，并将其存入一个向量中。成功返回 true，否则返回
     * false。
     * @param[in] image         带有中心区域的图片
     * @param[in] lightlines    存储的灯条向量
     * @param[in] globalRoi     全局 roi
     * @param[in] localRoi      局部 roi
     * @return true
     * @return false
     */
    bool findCenterLightlines(const cv::Mat &image, std::vector<Lightline> &lightlines,
                              const cv::Rect2f &globalRoi, const cv::Rect2f &localRoi)
    {
        // 寻找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(image, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
        for (const auto &contour : contours)
        {
            Lightline lightline(contour, globalRoi, localRoi);
            // 判断面积
            if (inRange(lightline.m_area, Param::MIN_CENTER_AREA, Param::MAX_CENTER_AREA) == false)
            {
                continue;
            }
            // 判断长宽比
            if (lightline.m_aspectRatio > Param::MAX_CENTER_ASPECT_RATIO)
            {
                continue;
            }
            // 如果全部符合，则存入向量中
            lightlines.emplace_back(std::move(lightline));
        }
        // 符合要求灯条的数量为 0 则失败
        if (lightlines.empty())
        {
            return false;
        }
        return true;
    }

    /**
     * @brief 根据中心灯条寻找并设置中心，成功返回 true，失败返回 false
     * @param[in] center        中心
     * @param[in] lightlines    中心灯条向量
     * @param[in] arrow         箭头
     * @param[in] armor         装甲板
     * @return true
     * @return false
     */
    bool findCenterR(CenterR &center, const std::vector<Lightline> &lightlines, const Arrow &arrow,
                     const Armor &armor)
    {
        // 设置中心 R 到装甲板中心的距离范围
        const double distance2ArmorCenter{armor.m_circlelight.m_radius * Param::POWER_RUNE_RADIUS / Param::ARMOR_RADIUS};
        const double ratio = 0.85;
        const double maxDistance2ArmorCenter{distance2ArmorCenter / ratio};
        const double minDistance2ArmorCenter{distance2ArmorCenter * ratio};
        // 设置中心 R 到箭头所在直线的最大距离
        const double maxDistance2ArrowLine{0.3 * armor.m_circlelight.m_radius};
        std::vector<Lightline> filteredLightlines;
        for (auto iter = lightlines.begin(); iter != lightlines.end(); ++iter)
        {
            double p2p{pointPointDistance(iter->m_center, armor.m_center)};
            double p2l{pointLineDistance(iter->m_center, armor.m_center, arrow.m_center)};
            // 判断到装甲板中心的距离
            if (inRange(p2p, minDistance2ArmorCenter, maxDistance2ArmorCenter) == false)
            {
                continue;
            }
            // 判断到箭头所在直线的距离
            if (p2l > maxDistance2ArrowLine)
            {
                continue;
            }
            filteredLightlines.push_back(*iter);
        }
        if (filteredLightlines.empty())
        {
            return false;
        }
        // 取所有符合要求的灯条中面积最大的为中心 R 灯条并设置中心 R
        Lightline target{
            *std::max_element(filteredLightlines.begin(), filteredLightlines.end(),
                              [](const Lightline &l1, const Lightline &l2)
                              { return l1.m_area < l2.m_area; })};
        center.set(target);
        return true;
    }

    /**
     * @brief 根据图像的大小调整 roi 位置，使其不越界导致程序终止
     * @param[in] rect          待调整的 roi
     * @param[in] mat           图像
     */
    void resetRoi(cv::Rect2f &rect, const cv::Mat &mat) { resetRoi(rect, mat.rows, mat.cols); }

    /**
     * @brief 根据图像的大小调整 roi 位置，使其不越界导致程序终止
     * @param[in] rect          待调整的 roi
     * @param[in] rows          行数
     * @param[in] cols          列数
     */
    void resetRoi(cv::Rect2f &rect, int rows, int cols)
    {
        // 调整左上角点的坐标
        rect.x = rect.x < 0 ? 0 : rect.x >= cols ? cols - 1
                                                 : rect.x;
        rect.y = rect.y < 0 ? 0 : rect.y >= rows ? rows - 1
                                                 : rect.y;
        // 调整长宽
        rect.width = rect.x + rect.width >= cols ? cols - rect.x - 1 : rect.width;
        rect.height = rect.y + rect.height >= rows ? rows - rect.y - 1 : rect.height;
        // 此时可能出现 width 或 height 小于 0 的情况，因此需要将其置为 0
        if (rect.width < 0)
        {
            rect.width = 0;
        }
        if (rect.height < 0)
        {
            rect.height = 0;
        }
    }

    void resetRoi(cv::Rect2f &rect, const cv::Rect2f &lastRoi) { resetRoi(rect, lastRoi.height, lastRoi.width); }

    /**
     * @brief 计算两个灯条长边的夹角
     * @param[in] l1
     * @param[in] l2
     * @return double
     */
    double calAngleBetweenLightlines(const Lightline &l1, const Lightline &l2)
    {
        // 长边对应方向向量
        std::array<std::array<cv::Point2f, 4>, 2> pointsArray;
        std::array<double, 2> lengths{l1.m_length, l2.m_length};
        l1.m_rotatedRect.points(pointsArray.at(0).begin());
        l2.m_rotatedRect.points(pointsArray.at(1).begin());
        std::array<cv::Point2f, 2> vecs;
        constexpr double eps = 1e-3;
        // 遍历灯条的四个点，寻找长边对应的两个点
        for (std::size_t i = 0; i < 2; ++i)
        {
            for (std::size_t j = 0; j < 4; ++i)
            {
                bool flag = false;
                for (std::size_t k = j; k < 4; ++j)
                {
                    if (std::abs(pointPointDistance(pointsArray.at(i).at(j), pointsArray.at(i).at(k)) -
                                 lengths.at(i)) < eps)
                    {
                        flag = true;
                        vecs.at(i) = pointsArray.at(i).at(j) - pointsArray.at(i).at(k);
                        break;
                    }
                }
                if (flag == true)
                {
                    break;
                }
            }
        }
        // 算向量之间夹角，取绝对值
        double dotProduct = vecs.at(0).x * vecs.at(1).x + vecs.at(0).y * vecs.at(1).y;
        double magnitude1 = std::sqrt(vecs.at(0).x * vecs.at(0).x + vecs.at(0).y * vecs.at(0).y);
        double magnitude2 = std::sqrt(vecs.at(1).x * vecs.at(1).x + vecs.at(1).y * vecs.at(1).y);
        double angle = radian2Angle(std::acos(dotProduct / (magnitude1 * magnitude2)));
        return angle;
    }

    /**
     * @brief 判断点是否在矩形内部（包括边界）
     * @param[in] point
     * @param[in] rect
     * @return true
     * @return false
     */
    bool inRect(const cv::Point2f &point, const cv::Rect2f &rect)
    {
        return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y &&
               point.y <= rect.y + rect.height;
    }

    /**
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     * @brief class Detector
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     *
     * @brief Construct a new Detector:: Detector object
     * @param[in] armor         装甲板
     * @param[in] center        中心 R
     */
    Detector::Detector()
        : // 全局 roi 初始设置为图像的全部范围，注意图像大小要与下面两个参数对应
          m_localMask{cv::Mat::zeros(Param::IMAGE_HEIGHT, Param::IMAGE_WIDTH, CV_8U)},
          m_globalRoi{0, 0, Param::IMAGE_WIDTH, Param::IMAGE_HEIGHT},
          m_startTime{std::chrono::steady_clock::now()},
          m_lightArmorNum{0}
    {
    }

    /**
     * @brief 检测箭头，装甲板和中心。如果所有检测均成功，则返回 true，否则返回 false。
     * @param[in] Frame        从相机传来的一帧图像，包括图像本身和其时间戳
     * @return true
     * @return false
     */
    bool Detector::detect(const Frame &frame)
    {
        bool reverse = false;
        preprocess(frame);
        if (detectArrow() == false)
        {
            m_status = Status::ARROW_FAILURE;
            goto FAIL;
        }
        setLocalRoi();
    RESTART:
        if (detectArmor() == false)
        {
            m_status = Status::ARMOR_FAILURE;
            goto FAIL;
        }
        if (detectCenterR() == false)
        {
            m_status = Status::CENTER_FAILURE;
            if (reverse == false)
            {
                std::swap(m_centerRoi, m_armorRoi);
                reverse = true;
                goto RESTART;
            }
            else
            {
                goto FAIL;
            }
        }
        setArmor();
        setGlobalRoi();
        m_status = Status::SUCCESS;
        return true;
    FAIL:
        // 如果检测失败，则将全局 roi 设为和原图片一样大小
        m_globalRoi = {0, 0, static_cast<float>(frame.m_image.cols), static_cast<float>(frame.m_image.rows)};
        m_lightArmorNum = 0;
        return false;
    }

    /**
     * @brief 预处理，包括图像的取 roi，通道分离及相减，二值化
     * @param[in] Frame        从相机传来的一帧图像，包括图像本身和其时间戳
     */
    void Detector::preprocess(const Frame &frame)
    {
        m_imageRaw = frame.m_image;
        m_imageShow = frame.m_image.clone();
        m_frameTime = frame.m_time;
        // 通道分离
        std::vector<cv::Mat> channels;
        cv::split(m_imageRaw, channels);
        // 对每个通道取 roi，加快处理速度
        cv::Mat blue{channels[0](m_globalRoi)}, red{channels[2](m_globalRoi)};
        // 通道相减得到灰度图。用己方颜色减去敌方颜色可以有效滤除白色区域
        cv::Mat temp;
        if (Param::COLOR == Color::RED)
        {
            temp = red - blue;
        }
        else
        {
            temp = blue - red;
        }
        // 对灰度图进行二值化
        cv::threshold(temp, m_imageArrow, Param::ARROW_BRIGHTNESS_THRESHOLD, Param::MAX_BRIGHTNESS,
                      cv::THRESH_BINARY);
        cv::threshold(temp, m_imageArmor, Param::ARMOR_BRIGHTNESS_THRESHOLD, Param::MAX_BRIGHTNESS,
                      cv::THRESH_BINARY);
#if SHOW_IMAGE >= 3
        cv::imshow("arrow binary", m_imageArrow);
        cv::imshow("armor binary", m_imageArmor);
#endif
        // 设置局部 roi
        m_localMask.setTo(0);
    }

    /**
     * @brief 寻找箭头，找到则返回 true，否则返回 false
     * @return true
     * @return false
     */
    bool Detector::detectArrow()
    {
        // 寻找符合箭头要求的灯条
        std::vector<Lightline> lightlines;
        findArrowLightlines(m_imageArrow, lightlines, m_globalRoi);
#if SHOW_IMAGE >= 2
        for (const auto &lightline : lightlines)
        {
            draw(lightline, Param::GREEN);
        }
#endif
        // 灯条匹配箭头
        if (findArrow(m_arrow, lightlines, m_globalRoi) == false)
        {
            // std::cout << "false" << std::endl;
            return false;
        }
#if SHOW_IMAGE >= 1
        draw(m_arrow.m_rotatedRect, Param::WHITE, 2);
#endif
        return true;
    }

    /**
     * @brief 设置局部 roi，局部 roi 包括中心 R 的 roi 和装甲板的 roi，根据箭头的两个端点进行提取。
     */
    void Detector::setLocalRoi()
    {
        // 设置两个 roi 矩形的距离和宽度
        double distance{m_arrow.m_length * Param::LOCAL_ROI_DISTANCE_RATIO};
        float width{Param::LOCAL_ROI_WIDTH};
        // 确定两个 roi 的中心点
        float x = distance * std::cos(angle2Radian(m_arrow.m_angle));
        float y = distance * std::sin(angle2Radian(m_arrow.m_angle));
        cv::Point2f centerUp{m_arrow.m_center.x - m_globalRoi.x + x, m_arrow.m_center.y - m_globalRoi.y + y};
        cv::Point2f centerDown{m_arrow.m_center.x - x - m_globalRoi.x, m_arrow.m_center.y - m_globalRoi.y - y};
        /**
         * 用类似旋转矩形先与原图做掩码，可以减少箭头灯条在装甲板区域的个数，避免箭头灯条与装甲板连在一起从而误识别的情况
         * 家里的符箭头灯条和装甲板灯条亮度差距过大，导致装甲板区域如果存在箭头灯条的话，二值化后的图像所有箭头灯条会和装甲板内部灯条连在一起，导致特征识别失败
         */
        cv::RotatedRect rectUp{centerUp, cv::Size(width, width), (float)m_arrow.m_angle};
        cv::RotatedRect rectDown{centerDown, cv::Size(width, width), (float)m_arrow.m_angle};

        std::array<std::array<cv::Point2f, 4>, 2> roiPoints;
        rectUp.points(roiPoints.at(0).begin());
        rectDown.points(roiPoints.at(1).begin());
        // 调整角点坐标不要越界，否则程序会直接中断退出
        for (auto &points : roiPoints)
        {
            for (auto &point : points)
            {
                if (point.x < 0)
                {
                    point.x = 0;
                }
                if (point.x > m_globalRoi.width)
                {
                    point.x = m_globalRoi.width;
                }
                if (point.y < 0)
                {
                    point.y = 0;
                }
                if (point.y > m_globalRoi.height)
                {
                    point.y = m_globalRoi.height;
                }
            }
        }
        /**
         * 注意 local mask 是和检测图像大小一致，检测部分为255，其余部分为0 的掩码，通过与检测图像进行与操作滤除
         * armor 和 center roi 是矩形，通过检测图像的裁剪得到目标图像
         */
        for (const auto &points : roiPoints)
        {
            // cv::fillConvexPoly 只支持 cv::Point 数组，因此在这里需要转换一下
            std::vector<cv::Point> _points;
            for (const auto &point : points)
            {
                _points.push_back(static_cast<cv::Point>(point));
            }
            cv::fillConvexPoly(m_localMask, _points, cv::Scalar(255, 255, 255));
        }
        m_armorRoi = cv::Rect2f(centerUp.x - width * 0.5, centerUp.y - width * 0.5, width, width);
        m_centerRoi = cv::Rect2f(centerDown.x - width * 0.5, centerDown.y - width * 0.5, width, width);
        // 调整 roi 不超过图像的边界
        resetRoi(m_armorRoi, m_globalRoi);
        resetRoi(m_centerRoi, m_globalRoi);
        // 如果上一帧中心 R 坐标不在中心 roi 中，则交换装甲板和中心 R 的 roi
        cv::Rect2f centerRoiGlobal{m_centerRoi.x + m_globalRoi.x, m_centerRoi.y + m_globalRoi.y,
                                   m_centerRoi.width, m_centerRoi.height};
        if (inRect(m_centerR.m_center, centerRoiGlobal) == false)
        {
            std::swap(m_armorRoi, m_centerRoi);
        }
        // 调换标志位，如果检测不到，则调换检测图像和备用图像，并将其置为 true
        draw(m_centerRoi, Param::DRAW_COLOR);
    }

    /**
     * @brief 寻找装甲板，找到则返回 true，否则为 false
     * @return true
     * @return false
     */
    bool Detector::detectArmor()
    {
        // armor roi 区域的图像为检测图像，center roi 区域为备用图像
        cv::Mat detect = (m_imageArmor & m_localMask)(m_armorRoi);
        cv::Mat backup = (m_imageArmor & m_localMask)(m_centerRoi);
        Circlelight circlelight;
        // 调换标志位，如果检测不到，则调换检测图像和备用图像，并将其置为 true
        bool reverse = false;
    RESTART:
        // 寻找符合装甲板边框要求的灯条
        if (findArmorCirclelight(detect, circlelight, m_globalRoi, m_armorRoi) == false)
        {
            // 如果找不到并且已经调换过图像了，则检测失败
            if (reverse == true)
            {
                return false;
            }
            // 如果找不到并且没有调换过，则调换图像并置标志位
            std::swap(detect, backup);
            std::swap(m_armorRoi, m_centerRoi);
            reverse = true;
            // 回到检测装甲板灯条处
            goto RESTART;
        }
#if SHOW_IMAGE >= 1
        cv::circle(m_imageShow, circlelight.m_center, circlelight.m_radius, Param::WHITE, 2);
        cv::circle(m_imageShow, circlelight.m_center, 1, Param::WHITE, 2);
#endif
        // 根据灯条匹配装甲板
        if (findArmor(m_armor, circlelight, m_arrow) == false)
        {
            if (reverse == true)
            {
                return false;
            }
            std::swap(detect, backup);
            std::swap(m_armorRoi, m_centerRoi);
            reverse = true;
            goto RESTART;
        }
        return true;
    }

    /**
     * @brief 寻找中心 R ，找到则返回 true，否则返回 false
     * @return true
     * @return false
     */
    bool Detector::detectCenterR()
    {
        m_imageCenter = (m_imageArmor & m_localMask)(m_centerRoi);
        // 寻找中心灯条，可能是多个
        std::vector<Lightline> lightlines;
        if (findCenterLightlines(m_imageCenter, lightlines, m_globalRoi, m_centerRoi) == false)
        {
            return false;
        }
#if SHOW_IMAGE >= 2
        for (const auto &lightline : lightlines)
        {
            draw(lightline, Param::YELLOW, 1, m_centerRoi);
        }
#endif
        // 从灯条中寻找中心 R
        if (findCenterR(m_centerR, lightlines, m_arrow, m_armor) == false)
        {
            return false;
        }
#if SHOW_IMAGE >= 1
        draw(m_centerR.m_boundingRect, Param::WHITE, 2, m_centerRoi);
#endif
        return true;
    }

    void Detector::setArmor()
    {
#if CONSOLE_OUTPUT >= 2
        MUTEX.lock();
        auto cameraPoints{getCameraPoints()};
        std::cout << "R标坐标:" << cameraPoints[4] << std::endl;
        std::cout << "装甲板中心坐标:" << m_armor.m_center << std::endl;
        MUTEX.unlock();
#endif
    }

    /**
     * @brief 设置全局 roi，选定符所在的区域，然后在下一个循环预处理时进行裁剪，可以加快运算时间，提高帧率
     */
    void Detector::setGlobalRoi()
    {
        double width{Param::GLOBAL_ROI_LENGTH_RATIO * 2 *
                     pointPointDistance(m_armor.m_center, m_centerR.m_center)};
        m_globalRoi = cv::Rect2f(m_centerR.m_x - 0.5 * width, m_centerR.m_y - 0.5 * width, width, width);
        resetRoi(m_globalRoi, Param::IMAGE_HEIGHT, Param::IMAGE_WIDTH);
#if SHOW_IMAGE >= 2
        cv::rectangle(m_imageShow, m_globalRoi, Param::DRAW_COLOR);
#endif
    }

    void Detector::drawTargetPoint(const cv::Point2f &point)
    {
        cv::circle(m_imageShow, point, 4, Param::DRAW_COLOR, 2);
    }

    /**
     * @brief 绘制灯条
     * @param[in] lightline     灯条
     * @param[in] color         颜色
     * @param[in] thickness     线条宽度
     * @param[in] localRoi      局部 roi
     */
    void Detector::draw(const Lightline &lightline, const cv::Scalar &color, const int thickness,
                        const cv::Rect2f &localRoi)
    {
        draw(lightline.m_rotatedRect, color, thickness, localRoi);
    }

    /**
     * @brief 绘制旋转矩形
     * @param[in] rotatedRect   旋转矩形
     * @param[in] color         颜色
     * @param[in] thickness     线条宽度
     * @param[in] localRoi      局部 roi
     */
    void Detector::draw(const cv::RotatedRect &rotatedRect, const cv::Scalar &color, const int thickness,
                        const cv::Rect2f &localRoi)
    {
        std::array<cv::Point2f, 4> vertices;
        rotatedRect.points(vertices.begin());
        draw(vertices.begin(), vertices.size(), color, thickness, localRoi);
    }

    /**
     * @brief 绘制正矩形
     * @param[in] rect          矩形
     * @param[in] color         颜色
     * @param[in] thickness     线条宽度
     * @param[in] localRoi      局部 roi
     */
    void Detector::draw(const cv::Rect2f &rect, const cv::Scalar &color, const int thickness,
                        const cv::Rect2f &localRoi)
    {
        cv::Rect2f temp = rect;
        temp.x += localRoi.x + m_globalRoi.x;
        temp.y += localRoi.y + m_globalRoi.y;
        cv::rectangle(m_imageShow, temp, color, thickness);
    }

    /**
     * @brief 绘制多边形，输入为向量
     * @param[in] points        多边形点集
     * @param[in] color         颜色
     * @param[in] thickness     线条宽度
     * @param[in] localRoi      局部 roi
     */
    void Detector::draw(const std::vector<cv::Point2f> &points, const cv::Scalar &color, const int thickness,
                        const cv::Rect2f &localRoi)
    {
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            cv::line(m_imageShow, points[i] + localRoi.tl() + m_globalRoi.tl(),
                     points[(i + 1) % points.size()] + localRoi.tl() + m_globalRoi.tl(), color, thickness);
        }
    }

    /**
     * @brief 绘制多边形，输入为 c 数组
     * @param[in] points        数组第一个元素的指针
     * @param[in] size          数组元素数量
     * @param[in] color         颜色
     * @param[in] thickness     线条宽度
     * @param[in] localRoi      局部 roi
     */
    void Detector::draw(const cv::Point2f *points, const std::size_t size, const cv::Scalar &color,
                        const int thickness, const cv::Rect2f &localRoi)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            cv::line(m_imageShow, points[i] + localRoi.tl() + m_globalRoi.tl(),
                     points[(i + 1) % size] + localRoi.tl() + m_globalRoi.tl(), color, thickness);
        }
    }

} // namespace rune
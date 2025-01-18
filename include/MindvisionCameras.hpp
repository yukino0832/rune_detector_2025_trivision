/**
 * 把迈德威视SDK限死只在这个头文件里使用
 * 迈德威视相机的专属使用类
 *
 * 首先，构造函数里做相机列表的初始化，调用CamerasInit初始化
 * 然后，功能函数有：
 *      CamerasInit()           负责用对应型号（或对应设备序列号）配置文件初始化列表里的所有相机。（第一版只有一个配置文件）
 *      CameraGetImage()        负责从特定相机中得到图片，在这里要可以用软触发模式
 */

#ifndef MINDVISIONCAMERA
#define MINDVISIONCAMERA

#include <CameraApi.h>

#include <opencv2/core/core.hpp>
#include <opencv2/calib3d.hpp>

#include <cstdio>
#include <iostream>
#include <map>
#include <vector>

// 你问我为什么要自己复制一个结构体，而不用SDK的tSdkCameraDevInfo？
// 因为中文编码出问题了……
struct MindvisionCameraDevInfo
{
    char acSn[32];          // 相机设备序列号
    char acProductName[32]; // 相机设备型号
    cv::Mat D;              // 相机畸变矩阵
    cv::Mat K;              // 相机内参矩阵
};

// 需要用到的单个相机的信息
struct MindvisionCamera
{
    int iStatus = -1;                // 相机设备状态
    int hCamera = -1;                // 相机设备句柄
    tSdkCameraCapbility tCapability; // 相机设备描述信息
    tSdkFrameHead sFrameInfo;        // 图像帧头信息
    unsigned char *pRawBuffer;       // 原始格式图像缓冲区
    unsigned char *pRbgBuffer;       // 24位位图格式图像缓冲区
    MindvisionCameraDevInfo DevInfo; // 相机设备信息（序列号、型号、畸变矩阵、内参矩阵）
    int the_th;                      // 这是第__个被枚举的相机（序号从0开始）
                                     // 这还对应了其所用的发布接口的编号，以及发布的话题名
                                     // 本质上是上电的顺序
};

class MindvisionCameras
{
public:
    /**
     * 初始化SDK
     * 检测枚举接上的迈德威视相机的列表
     * 相机列表的长度写死为6
     * 整个进程只能构造一次
     */
    explicit MindvisionCameras()
        : iCameraCounts_(6)
    {
        // 初始化迈德威视SDK
        CameraSdkInit(1);

        // 枚举设备，并建立设备列表
        CameraEnumerateDevice(tCameraEnumList_, &iCameraCounts_);
        // 如果没有设备连接，则退出
        if (iCameraCounts_ == 0)
        {
            throw std::runtime_error("没有发现有相机设备连接……\n");
        }
        printf("相机数量：%i\n", iCameraCounts_);
        std::flush(std::cout);

        D_dic_["048051610265"] = (cv::Mat_<double>(cv::Size(5, 1)) << -0.3147645554215756, 0.0661956290059946, 0.001468210073478912, -0.0002062873237225103, 0.0);
        D_dic_["048061610028"] = (cv::Mat_<double>(cv::Size(5, 1)) << -0.3096855767177183, 0.0653738928528381, 0.0005387775411323192, 0.0005373694145995487, 0.0);
        D_dic_["048061610044"] = (cv::Mat_<double>(cv::Size(5, 1)) << -0.30063308501375136, 0.059399376173195885, 0.0006605563122632131, 0.00017374160383460858, 0.0);
        D_dic_["048061610048"] = (cv::Mat_<double>(cv::Size(5, 1)) << -0.302064969300824, 0.05815734648549415, 0.0002941434163204548, -0.0018530146178703949, 0.0);

        K_dic_["048051610265"] = (cv::Mat_<double>(cv::Size(3, 3)) << 690.0730153001708, 0.0, 652.487593047458, 0.0, 696.0569628959367, 503.3331339074178, 0.0, 0.0, 1.0);
        K_dic_["048061610028"] = (cv::Mat_<double>(cv::Size(3, 3)) << 708.7050483705385, 0.0, 641.7548969754414, 0.0, 714.9819028916913, 507.6058430417861, 0.0, 0.0, 1.0);
        K_dic_["048061610044"] = (cv::Mat_<double>(cv::Size(3, 3)) << 681.0111891470904, 0.0, 659.6906793034907, 0.0, 687.6814441275018, 470.7970306924219, 0.0, 0.0, 1.0);
        K_dic_["048061610048"] = (cv::Mat_<double>(cv::Size(3, 3)) << 685.0936959336213, 0.0, 647.6866143180478, 0.0, 691.41964817053, 458.6596995199036, 0.0, 0.0, 1.0);

        // 初始化列表里的所有相机设备
        CamerasInit();
    }

    ~MindvisionCameras()
    {
        for (int i = 0; i < iCameraCounts_; ++i)
        {
            // 相机用完了要进行反初始化
            CameraUnInit(Cameras_[i].hCamera);
            printf("第%i号相机反初始化完成\n", i + 1);
            std::flush(std::cout);
            // 并释放24位位图格式的缓存区
            // 一定要先反初始化，再释放内存
            free(Cameras_[i].pRbgBuffer);
        }
    }

    int GetDeviceNumber()
    {
        return iCameraCounts_;
    }

    cv::Mat CameraGetImage(std::string acSn)
    {
        int i = th_dic_[acSn];
        // printf("?????");
        // std::flush(std::cout);
        if (CameraGetImageBuffer(Cameras_[i].hCamera, &Cameras_[i].sFrameInfo, &Cameras_[i].pRawBuffer, 1000) == CAMERA_STATUS_SUCCESS)
        {
            // 把原始格式的图像处理为24位位图格式
            CameraImageProcess(Cameras_[i].hCamera, Cameras_[i].pRawBuffer, Cameras_[i].pRbgBuffer, &Cameras_[i].sFrameInfo);
            // 把相机数据格式的位图转成OpenCV的Mat
            cv::Mat MatImage(
                cv::Size(Cameras_[i].sFrameInfo.iWidth, Cameras_[i].sFrameInfo.iHeight),
                Cameras_[i].sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
                Cameras_[i].pRbgBuffer);
            // 释放相机图像缓存区
            // 在成功调用CameraGetImageBuffer后，必须调用CameraReleaseImageBuffer来释放获得的buffer
            // 否则再次调用CameraGetImageBuffer时，程序将被挂起一直阻塞，直到其他线程中调用CameraReleaseImageBuffer来释放了buffer
            CameraReleaseImageBuffer(Cameras_[i].hCamera, Cameras_[i].pRawBuffer);

            return MatImage;
        }
        else
        {
            cv::Mat empty;
            return empty;
        }
    }

private:
    int iCameraCounts_;                     // 实际连接上的相机设备数
    tSdkCameraDevInfo tCameraEnumList_[6];  // SDK的相机设备枚举列表
    std::vector<MindvisionCamera> Cameras_; // 初始化后的相机信息列表

    std::map<std::string, cv::Mat> D_dic_; // 相机畸变参数字典
    std::map<std::string, cv::Mat> K_dic_; // 相机内参矩阵字典
    std::map<std::string, int> th_dic_;    // 相机设备枚举列表虚拟序列号字典

    void CamerasInit()
    {
        for (int i = 0; i < iCameraCounts_; ++i)
        {

            // 往容器里塞个变量，不然段错误了
            MindvisionCamera camera;
            Cameras_.push_back(camera);

            // 这是第__个被枚举的相机（序号从0开始）
            Cameras_[i].the_th = i;

            // 相机设备枚举列表里的对应相机进行初始化
            // 初始化成功后，才能调用任何其他相机相关的操作接口
            Cameras_[i].iStatus = CameraInit(&tCameraEnumList_[i], PARAM_MODE_BY_SN, PARAMETER_TEAM_DEFAULT, &Cameras_[i].hCamera);
            if (Cameras_[i].iStatus != CAMERA_STATUS_SUCCESS)
            {
                printf("相机设备枚举列表的第%i号相机初始化失败，相机序列号%s，相机型号%s，相机状态报错号为%i\n", i + 1, Cameras_[i].DevInfo.acSn, Cameras_[i].DevInfo.acProductName, Cameras_[i].iStatus);
                std::flush(std::cout);
                continue;
            }
            // 获得相机的特性描述结构体
            CameraGetCapability(Cameras_[i].hCamera, &Cameras_[i].tCapability);
            // 设置相机的设备信息到迈德威视相机的结构体
            strcpy(Cameras_[i].DevInfo.acSn, tCameraEnumList_[i].acSn);                   // 设置序列号
            strcpy(Cameras_[i].DevInfo.acProductName, tCameraEnumList_[i].acProductName); // 设置产品型号
            Cameras_[i].DevInfo.D = D_dic_[Cameras_[i].DevInfo.acSn];
            Cameras_[i].DevInfo.K = K_dic_[Cameras_[i].DevInfo.acSn];
            th_dic_[Cameras_[i].DevInfo.acSn] = i;

            // 24位位图格式的缓冲区，要我分配内存
            // 但原始格式的缓冲区，不需要我分配内存呢
            Cameras_[i].pRbgBuffer = (unsigned char *)malloc(Cameras_[i].tCapability.sResolutionRange.iHeightMax * Cameras_[i].tCapability.sResolutionRange.iWidthMax * 3);

            // 让SDK进入工作模式，开始接收来自相机収送的图像
            // 如果当前相机是触发模式，则需要接收到触发帧以后才会更新图像
            CameraPlay(Cameras_[i].hCamera);

            /* 总之这里配置文件应该根据相机选取，并且这个路径很可能出问题，改软编码 */
            // 用配置文件导入相机参数
            std::string File = "../Camera/Configs/" + std::string(Cameras_[i].DevInfo.acSn) + ".config";
            Cameras_[i].iStatus = CameraReadParameterFromFile(Cameras_[i].hCamera, File.data());
            // 万一文件位置错了
            if (Cameras_[i].iStatus != CAMERA_STATUS_SUCCESS)
            {
                printf("相机设备枚举列表的第%i号相机设置参数失败，相机状态报错号为%i\n", i + 1, Cameras_[i].iStatus);
                std::flush(std::cout);
                continue;
            }

            // 设置图像处理的输出格式，彩色黑白都支持RGB24位
            // 这个居然在配置文件中没有设置？
            if (Cameras_[i].tCapability.sIspCapacity.bMonoSensor)
            {
                CameraSetIspOutFormat(Cameras_[i].hCamera, CAMERA_MEDIA_TYPE_MONO8);
            }
            else
            {
                CameraSetIspOutFormat(Cameras_[i].hCamera, CAMERA_MEDIA_TYPE_BGR8);
            }
            printf("第%i个上电的相机初始化及配置成功，对应设备的序列号为%s，设备型号为%s\n", i + 1, Cameras_[i].DevInfo.acSn, Cameras_[i].DevInfo.acProductName);
            std::flush(std::cout);
        }
    }
};

#endif

#include <iostream>
#include <fstream>


#include <Windows.h>
#include <chrono>
#include "opencv2/core.hpp"
#include "opencv2/video.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp" 
#include <opencv2/core/utils/logger.hpp>
#define NAME_PROG "ConsolePlayer"
HWND MAIN_HWND = 0;
HANDLE MAIN_CONSOLE = 0;
void GetAndSetWindowHnalder(const char* _console)
{
    auto getHwnd = []()->HWND {
        const int BUFSIZE = 1024;
        WCHAR windowTitle[BUFSIZE];
        GetConsoleTitle(windowTitle, BUFSIZE);
        return FindWindow(NULL, windowTitle);
    };
    MAIN_HWND = getHwnd();
    SetWindowTextA(MAIN_HWND, _console);
    MAIN_CONSOLE = GetStdHandle(STD_OUTPUT_HANDLE);
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);
    /*ShowConsoleCursor(false);*/
    auto disableConsoleCursor = []() {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(MAIN_CONSOLE, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(MAIN_CONSOLE, &cursorInfo);
    };
    disableConsoleCursor();
}
static void SetCmdSizeLoc(cv::Size size)
{
    ShowWindow(MAIN_HWND, SW_MAXIMIZE);
    ShowScrollBar(MAIN_HWND, SB_BOTH, FALSE);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    bool res = SetWindowPos(
        MAIN_HWND,
        HWND_TOP,
        (width - static_cast<int>(roundf(((float)height / 2.0 * 2.0) * ((float)size.width / (float)size.height))) - GetSystemMetrics(SM_CXVSCROLL)) / 2,
        0,
        static_cast<int>(roundf(((float)height / 2.0 * 2.0) * ((float)size.width / (float)size.height))) - GetSystemMetrics(SM_CXVSCROLL),
        height,
        SWP_SHOWWINDOW
    );
    return;
}
inline void SetFontSize(SHORT size)
{
    CONSOLE_FONT_INFOEX cfi = { sizeof(cfi) };
    GetCurrentConsoleFontEx(MAIN_CONSOLE, FALSE, &cfi);
    cfi.dwFontSize.Y = size;
    SetCurrentConsoleFontEx(MAIN_CONSOLE, FALSE, &cfi);

    auto disableConsoleCursor = []() {
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(MAIN_CONSOLE, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(MAIN_CONSOLE, &cursorInfo);
    };
    disableConsoleCursor();
    return;
}

typedef struct cmdFrameString{
    std::string cdat_;
    long long tic_ = 0;
}cmdFrameStringDef;
typedef struct cmdFrame{
	cv::Mat dat_;
    long long tic_ = 0;
}cmdFrameDef;
typedef std::vector<cmdFrameDef> cmdVideoDef;
typedef std::vector<cmdFrameStringDef> cmdVideoStringDef;
void LoadFrames(cmdVideoDef& _video, std::string& _videoFilePath)
{
    try
    {
        cv::VideoCapture cap(_videoFilePath);
        if (!cap.isOpened())
            std::cout << "Can not open video file" << std::endl;

        for (int frameNum = 0; frameNum < cap.get(cv::CAP_PROP_FRAME_COUNT); frameNum++)
        {
            
            cmdFrameDef tframe;
            tframe.tic_ = cap.get(cv::CAP_PROP_POS_MSEC);
            cap >> tframe.dat_;
            _video.push_back(tframe);
         /*   std::cout << "framen:" << frameNum << std::endl;*/
        }
    }
    catch (cv::Exception& e)
    {
        std::cout << e.msg << std::endl;
    }
}
void VideoFramePixelization(cmdVideoDef& _dstVideoContent, cmdVideoDef& _srcVideoContent, cv::Size size_pixel)
{
    
    _dstVideoContent.clear();
    for (auto frame: _srcVideoContent){
        // resize each frame using INTER_LINEAR principle
        cv::Mat mat;
        cmdFrameDef tmpFrame;
        tmpFrame.tic_ = frame.tic_;

        cv::resize(frame.dat_, mat,  size_pixel, 0, 0, cv::INTER_LINEAR);
        cv::cvtColor(mat, tmpFrame.dat_, cv::COLOR_BGR2GRAY);

        _dstVideoContent.push_back(tmpFrame);
    }
}
#include "shades.h"
inline void CreateStringByFrame(cv::Mat& frame, std::string& result)
{
    for (int i = 0; i < frame.rows; i++)
    {
        for (int j = 0; j < frame.cols; j++)
        {
            result.push_back(Shades::SHADES[static_cast<int>(frame.at<uchar>(i, j))]);
            result.append(" ");
        }
        result.append("\n");
    }
}
void CreateVectorFrameString(cmdVideoDef& src, cmdVideoStringDef& dst)
{
    for (auto it : src)
    {
        cmdFrameStringDef frame_string;

        frame_string.tic_ = it.tic_;
        CreateStringByFrame(it.dat_, frame_string.cdat_);

        dst.push_back(frame_string);
    }
}


int GetFrameIndexByTime(long long _time, cmdVideoStringDef& _content)
{
    static int index_last_frame = 0;
    for (int i = index_last_frame; i < _content.size(); i++)
    {
        if (_content[i].tic_ >= _time)
        {
            index_last_frame = i;
            return i;
        }
    }
    return -1;
}
void Play(cmdVideoStringDef& _videoContentString)
{
    int last_frame = INT_MAX;

    auto time_start = std::chrono::system_clock::now();

    while (true)
    {
        int current_frame = GetFrameIndexByTime((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time_start)).count(), _videoContentString);

        if (last_frame == current_frame)
        {
            continue;
        }

        last_frame = current_frame;

        if (current_frame == -1)
            break;

        auto GoToPosition = [](int x = 0, int y = 0) -> void{
                COORD coord{ x, y };
                SetConsoleCursorPosition(MAIN_CONSOLE, coord);
        };
        GoToPosition(0,0);

        std::cout << _videoContentString[current_frame].cdat_ << std::flush;
    }
}
void SetFontForPlay(cv::Size size)
{
    RECT rect;
    GetWindowRect(MAIN_HWND, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    SetFontSize(height / size.height);

    return;
}


static int ParseFilePathFromMainParams(std::string& _filePath, int _argc, char* _argv[])
{
    // Barrier
    if (_argc == 1) {
        std::cout << "Error: parameter insufficient, single parameter at least!" << std::endl;
        return 1;
    }
    if (_argc > 2) {
        std::cout << "Error: only single parameter expected!" << std::endl;
    }
    // Open file by path
    _filePath = std::string(_argv[1]);
    std::ifstream file = std::ifstream(_filePath.c_str());
    // File existence check
    auto fileExist = [](std::ifstream& _file)-> bool {
        return _file.good();
    };

    if (!fileExist(file))
    {
        std::cout << "Error: File [" << _filePath << "] not found!" << std::endl;
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
	// Get file path
    std::string videoFilePath;
    ParseFilePathFromMainParams(videoFilePath, argc, argv);
    
    // Displayer layout
    #define CONSOLEANME "ConsolePlayer"
    GetAndSetWindowHnalder(CONSOLEANME);
    cv::Size frameSize{ 100, 100 };
    SetCmdSizeLoc(frameSize);

    // Loading frmes
    cmdVideoDef videoContentA;// = new cmdVideoDef();
    cmdVideoDef videoContentB;// = new cmdVideoDef();
    LoadFrames(videoContentA, videoFilePath);
    // Pixelization
    VideoFramePixelization(videoContentB, videoContentA, frameSize);
    // Mapping
    cmdVideoStringDef videoContentString;
    CreateVectorFrameString(videoContentB, videoContentString);
    // Play
    SetFontForPlay(frameSize);
    system("cls");
    std::cout << "Press enter to start";
    std::getchar();
    system("cls");
    //std::cout << videoContentString[0].cdat_ << std::flush;
    Play(videoContentString);

    return 0;
}

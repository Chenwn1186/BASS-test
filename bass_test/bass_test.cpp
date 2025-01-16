/*
    WASAPI 版本的 BASS 简单控制台播放器
    版权所有 (c) 1999-2023 Un4seen Developments Ltd.
*/

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "basswasapi.h"
#include "bassmix.h"
#include "bass.h"

HSTREAM mixer;

// 显示错误信息
void Error(const char *text)
{
    printf("错误(%d): %s\n", BASS_ErrorGetCode(), text);
    BASS_WASAPI_Free();
    BASS_Free();
    exit(0);
}

// WASAPI 回调函数
DWORD CALLBACK WasapiProc(void *buffer, DWORD length, void *user)
{
    return BASS_ChannelGetData(mixer, buffer, length);
}

// 列出可用设备
void ListDevices()
{
    BASS_WASAPI_DEVICEINFO di;
    int a;
    for(a = 0; BASS_WASAPI_GetDeviceInfo(a, &di); a++) {
        if((di.flags & BASS_DEVICE_ENABLED) && !(di.flags & BASS_DEVICE_INPUT)) // 启用的输出设备
            printf("设备 %d: %s\n", a, di.name);
    }
}

int main()
{
    // 解释流程：
    // 1. 初始化 BASS 库并检查其版本。
    // 2. 获取默认的 WASAPI 输出设备。
    // 3. 初始化 WASAPI 设备。
    // 4. 尝试从固定的音乐文件路径加载音频文件。
    // 5. 创建混音器并将音频流添加到混音器中。
    // 6. 开始播放音频。
    // 7. 监测播放状态，直至用户按下任意键。

    const char *musicFile = "D:\\FlutterProjects\\AGA-孤雏.mp3"; // 定义固定的音乐文件路径
    DWORD chan;
    BASS_CHANNELINFO info;
    BASS_WASAPI_DEVICEINFO dinfo;
    DWORD flags = BASS_WASAPI_AUTOFORMAT | BASS_WASAPI_BUFFER; // 默认初始化标志

    printf("BASSWASAPI 简单控制台播放器\n"
        "--------------------------------\n");

    // 检查 BASS 库的版本
    if(HIWORD(BASS_GetVersion()) != BASSVERSION) {
        printf("加载了不正确的 BASS 版本");
        return 0;
    }

    int device = -1; // 默认设备
    // 获取默认输出设备
    for(int a = 0; BASS_WASAPI_GetDeviceInfo(a, &dinfo); a++) {
        if((dinfo.flags & (BASS_DEVICE_DEFAULT | BASS_DEVICE_LOOPBACK | BASS_DEVICE_INPUT)) == BASS_DEVICE_DEFAULT) {
            device = a;
            break;
        }
    }
    if(device == -1) Error("无法找到输出设备");

    // 由于未通过 BASS 播放任何内容，因此不需要更新线程
    BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

    // 设置 BASS - "无声音" 设备，使用 "混音" 采样率
    BASS_Init(0, dinfo.mixfreq, 0, 0, NULL);

    // 尝试加载音乐文件
    chan = BASS_StreamCreateFile(FALSE, musicFile, 0, 0, BASS_SAMPLE_FLOAT | BASS_SAMPLE_LOOP | BASS_STREAM_DECODE);
    if(!chan) Error("无法播放文件");

    BASS_ChannelGetInfo(chan, &info);
    printf("类型: %x\n", info.ctype);
    if(info.origres)
        printf("格式: %u Hz, %d 通道, %d 位\n", info.freq, info.chans, LOWORD(info.origres));
    else
        printf("格式: %u Hz, %d 通道\n", info.freq, info.chans);

    { // 设置输出
        BASS_WASAPI_INFO wi;
        // 初始化 WASAPI 设备
        if(!BASS_WASAPI_Init(device, info.freq, info.chans, flags, 0.4, 0.05, WasapiProc, NULL))
            Error("无法初始化设备");

        // 获取输出详细信息
        BASS_WASAPI_GetInfo(&wi);
        printf("输出: %s%s 模式, %d Hz, %d 通道\n",
            wi.initflags & BASS_WASAPI_EVENT ? "事件驱动 " : "", wi.initflags & BASS_WASAPI_EXCLUSIVE ? "独占模式" : "共享模式", wi.freq, wi.chans);

        // 创建一个与采样格式相同的混音器并启用 GetPositionEx
        mixer = BASS_Mixer_StreamCreate(wi.freq, wi.chans, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_POSEX);

        // 将源添加到混音器中（如有必要，进行下混音）
        BASS_Mixer_StreamAddChannel(mixer, chan, BASS_MIXER_DOWNMIX);

        // 开始播放
        if(!BASS_WASAPI_Start())
            Error("无法启动输出");
    }

    // 循环监控播放状态，直到用户按下任意键
    while(!_kbhit() && BASS_ChannelIsActive(mixer)) {
        // 显示一些信息并稍等
        Sleep(50);
    }

    printf("                                                                             \n");

    BASS_WASAPI_Free();
    BASS_Free();
    return 0;
}

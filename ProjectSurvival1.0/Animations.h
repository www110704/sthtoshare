#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<graphics.h>
#pragma comment(lib,"MSIMG32.lib")//AlphaBlend函数
#pragma comment(lib,"Winmm.lib")//播放音效静态库 mciSendString()函数--》Media Control Interface

const int windows_width = 1280;
const int window_height = 720;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	//渲染含有透明像素的位图
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}


class Animation
{
public:
	Animation(LPCTSTR path,int num,int interval) 
		:interval_ms(interval), frame_list(std::vector<IMAGE*>(num,nullptr)), timer(0), idx_frame(0)
	{
		//载入动画帧
		TCHAR file_path[256];

		for (size_t i = 0; i < num; ++i) {
			_stprintf_s(file_path, path, i);//路径名+文件名通用部分+编号


			IMAGE* frame = new IMAGE();//在哪里使用delete？？？
			loadimage(frame, file_path);
			frame_list[i] = frame;
		}
	}

	virtual ~Animation() 
	{
		for (IMAGE* img : frame_list) {
			delete img;
		}
	}

	void play(int x,int y,int delay_time) {//坐标和距离上次调用play时间间隔--》将计数器变为计时器--》从而与游戏的帧数脱钩
		timer += delay_time;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
		}

		putimage_alpha(x,y, frame_list[idx_frame]);
	}

private:



private:
	int timer;//动画计时器
	int idx_frame;//动画帧索引
	int interval_ms;//帧切换的时间间隔
	std::vector<IMAGE*> frame_list;//很危险，若拷贝了数组怎么办?-->智能指针

};
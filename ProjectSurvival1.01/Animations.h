#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<graphics.h>
#pragma comment(lib,"MSIMG32.lib")//AlphaBlend函数
#pragma comment(lib,"Winmm.lib")//播放音效静态库 mciSendString()函数--》Media Control Interface

const int windows_width = 1280;
const int window_height = 720;
const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;

bool is_game_started = false;
bool running = true;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	//渲染含有透明像素的位图
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

//享元模式----共享元素
class Atlas {
public:
	Atlas(LPCTSTR path, int num)
		:frame_list(std::vector<IMAGE*>(num, nullptr))
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
	~Atlas()
	{
		for (IMAGE* img : frame_list) {
			delete img;
		}
	}

	std::vector<IMAGE*> frame_list;//很危险，若拷贝了数组怎么办?-->智能指针
};
//定义全局享变量
Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;


class Animation
{
public:
	Animation(Atlas* atlas,int interval) 
	{
		anim_atlas = atlas;
		interval_ms = interval;
	}

	~Animation() = default;


	void play(int x,int y,int delay_time) {//坐标和距离上次调用play时间间隔--》将计数器变为计时器--》从而与游戏的帧数脱钩
		timer += delay_time;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}

		putimage_alpha(x,y, anim_atlas->frame_list[idx_frame]);
	}


private:
	int timer = 0;//动画计时器
	int idx_frame = 0;//动画帧索引
	int interval_ms;//帧切换的时间间隔
	
	Atlas* anim_atlas;//指向已有资源
};
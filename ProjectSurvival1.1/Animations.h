#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<graphics.h>
#include"Utils.h"
#include"Timer.h"

extern int windows_width;
extern int window_height;
extern int BUTTON_WIDTH;
extern int BUTTON_HEIGHT;



//享元模式----共享元素
class Atlas {
public:
	Atlas(LPCTSTR path, int num)
		:frame_list(std::vector<IMAGE*>(num, nullptr))
	{
		//载入动画帧
		if (num == 1) 
		{
			IMAGE* frame = new IMAGE();
			loadimage(frame, path);
			frame_list[0] = frame;
		}
		else 
		{
			TCHAR file_path[256];
			for (size_t i = 0; i < num; ++i) {
				_stprintf_s(file_path, path, i);//路径名+文件名通用部分+编号

				IMAGE* frame = new IMAGE();//在哪里使用delete？？？
				loadimage(frame, file_path);
				frame_list[i] = frame;
			}
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



class Animation
{
public:
	Animation(Atlas* atlas,int interval) 
	{
		anim_atlas = atlas;
		interval_ms = interval;
	}

	~Animation() = default;

	void set_loop(bool flag) {
		isloop = flag;
	}

	//不循环且运行到最后一帧
	bool checkfinish() {
		if (isloop) return false;
		return idx_frame == anim_atlas->frame_list.size() - 1;
	}

	void on_update(int delay) {
		timer += delay;
		if (timer >= interval_ms) {
			++idx_frame;
			timer = 0;
			if (idx_frame >= anim_atlas->frame_list.size()) {
				idx_frame = isloop ? 0 : (int)anim_atlas->frame_list.size() - 1;
				if (!isloop && callback) callback();
			}
		}
	}

	void on_draw(int x,int y) {//坐标和距离上次调用play时间间隔--》将计数器变为计时器--》
		putimage_alpha(x,y, anim_atlas->frame_list[idx_frame]);
	}

	void set_callback(std::function<void()> callback) {
		this->callback = callback;
	}

	//void play(int x, int y, int delay_time) {//坐标和距离上次调用play时间间隔--》将计数器变为计时器--》从而与游戏的帧数脱钩
	//	timer += delay_time;
	//	if (timer >= interval_ms) {
	//		idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
	//		timer = 0;
	//	}

	//	putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	//}


private:
	int timer = 0;//动画计时器
	int idx_frame = 0;//动画帧索引
	int interval_ms;//帧切换的时间间隔
	bool isloop = true;
	
	Atlas* anim_atlas;//指向已有资源
	std::function<void()> callback;//回调函数
};
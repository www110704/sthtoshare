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



//��Ԫģʽ----����Ԫ��
class Atlas {
public:
	Atlas(LPCTSTR path, int num)
		:frame_list(std::vector<IMAGE*>(num, nullptr))
	{
		//���붯��֡
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
				_stprintf_s(file_path, path, i);//·����+�ļ���ͨ�ò���+���

				IMAGE* frame = new IMAGE();//������ʹ��delete������
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

	std::vector<IMAGE*> frame_list;//��Σ�գ���������������ô��?-->����ָ��
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

	//��ѭ�������е����һ֡
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

	void on_draw(int x,int y) {//����;����ϴε���playʱ����--������������Ϊ��ʱ��--��
		putimage_alpha(x,y, anim_atlas->frame_list[idx_frame]);
	}

	void set_callback(std::function<void()> callback) {
		this->callback = callback;
	}

	//void play(int x, int y, int delay_time) {//����;����ϴε���playʱ����--������������Ϊ��ʱ��--���Ӷ�����Ϸ��֡���ѹ�
	//	timer += delay_time;
	//	if (timer >= interval_ms) {
	//		idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
	//		timer = 0;
	//	}

	//	putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	//}


private:
	int timer = 0;//������ʱ��
	int idx_frame = 0;//����֡����
	int interval_ms;//֡�л���ʱ����
	bool isloop = true;
	
	Atlas* anim_atlas;//ָ��������Դ
	std::function<void()> callback;//�ص�����
};
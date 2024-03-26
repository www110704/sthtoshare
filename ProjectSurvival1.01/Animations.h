#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<graphics.h>
#pragma comment(lib,"MSIMG32.lib")//AlphaBlend����
#pragma comment(lib,"Winmm.lib")//������Ч��̬�� mciSendString()����--��Media Control Interface

const int windows_width = 1280;
const int window_height = 720;
const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;

bool is_game_started = false;
bool running = true;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	//��Ⱦ����͸�����ص�λͼ
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}

//��Ԫģʽ----����Ԫ��
class Atlas {
public:
	Atlas(LPCTSTR path, int num)
		:frame_list(std::vector<IMAGE*>(num, nullptr))
	{
		//���붯��֡
		TCHAR file_path[256];
		for (size_t i = 0; i < num; ++i) {
			_stprintf_s(file_path, path, i);//·����+�ļ���ͨ�ò���+���

			IMAGE* frame = new IMAGE();//������ʹ��delete������
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

	std::vector<IMAGE*> frame_list;//��Σ�գ���������������ô��?-->����ָ��
};
//����ȫ�������
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


	void play(int x,int y,int delay_time) {//����;����ϴε���playʱ����--������������Ϊ��ʱ��--���Ӷ�����Ϸ��֡���ѹ�
		timer += delay_time;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}

		putimage_alpha(x,y, anim_atlas->frame_list[idx_frame]);
	}


private:
	int timer = 0;//������ʱ��
	int idx_frame = 0;//����֡����
	int interval_ms;//֡�л���ʱ����
	
	Atlas* anim_atlas;//ָ��������Դ
};
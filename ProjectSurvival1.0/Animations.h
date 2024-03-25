#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<graphics.h>
#pragma comment(lib,"MSIMG32.lib")//AlphaBlend����
#pragma comment(lib,"Winmm.lib")//������Ч��̬�� mciSendString()����--��Media Control Interface

const int windows_width = 1280;
const int window_height = 720;

inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	//��Ⱦ����͸�����ص�λͼ
	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
}


class Animation
{
public:
	Animation(LPCTSTR path,int num,int interval) 
		:interval_ms(interval), frame_list(std::vector<IMAGE*>(num,nullptr)), timer(0), idx_frame(0)
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

	virtual ~Animation() 
	{
		for (IMAGE* img : frame_list) {
			delete img;
		}
	}

	void play(int x,int y,int delay_time) {//����;����ϴε���playʱ����--������������Ϊ��ʱ��--���Ӷ�����Ϸ��֡���ѹ�
		timer += delay_time;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % frame_list.size();
			timer = 0;
		}

		putimage_alpha(x,y, frame_list[idx_frame]);
	}

private:



private:
	int timer;//������ʱ��
	int idx_frame;//����֡����
	int interval_ms;//֡�л���ʱ����
	std::vector<IMAGE*> frame_list;//��Σ�գ���������������ô��?-->����ָ��

};
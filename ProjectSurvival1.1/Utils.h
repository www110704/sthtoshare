#pragma once

#include <graphics.h>

#pragma comment(lib,"MSIMG32.lib")//AlphaBlend����
#pragma comment(lib,"Winmm.lib")//������Ч��̬�� mciSendString()����--��Media Control Interface

inline void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();
	int h = img->getheight();
	//��Ⱦ����͸�����ص�λͼ
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}

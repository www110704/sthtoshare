#pragma once

#include <graphics.h>

#pragma comment(lib,"MSIMG32.lib")//AlphaBlend函数
#pragma comment(lib,"Winmm.lib")//播放音效静态库 mciSendString()函数--》Media Control Interface

inline void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();
	int h = img->getheight();
	//渲染含有透明像素的位图
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}

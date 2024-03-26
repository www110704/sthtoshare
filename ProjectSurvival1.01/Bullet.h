#pragma once
#include"Animations.h"


class Bullet
{
public:
	Bullet() = default;
	~Bullet() = default;

	void Draw() 
	{
		//使用橙色填充圆进行绘制
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x,position.y,RADIUS);
	}

public:
	POINT position = { 0,0 };

private:
	int RADIUS = 10;
};
#pragma once
#include"Animations.h"

int BUTTON_WIDTH = 192;
int BUTTON_HEIGHT = 75;

extern Atlas* atlas_pea_idle;
extern Atlas* atlas_pea_break;

class Bullet
{
public:
	Bullet() = default;
	~Bullet() = default;

	virtual void Draw() 
	{
		//使用橙色填充圆进行绘制
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x,position.y,RADIUS);
	}

	const POINT& GetPosition() const{
		return position;
	}

	void SetPosition(int x,int y) {
		position.x = x;
		position.y = y;
	}
	

private:
	POINT position = { 0,0 };//绘制的子弹位置在子弹中心
	int RADIUS = 10;
};


class PeaBullet:public Bullet
{
public:

	PeaBullet(POINT pos,POINT des) 
		:position(pos)
	{
		destination.x = des.x - img_width/2;//计算子弹大小
		destination.y = des.y - img_height/2;

		//计算移动方向
		int dir_x = destination.x - position.x;
		int dir_y = destination.y - position.y;
		double move_len = sqrt(dir_x * dir_x + dir_y * dir_y);//移动总距离
		normalize_x = dir_x / move_len;//x方向移动的归一化距离
		normalize_y = dir_y / move_len;//y方向移动的归一化距离


		//加载动画
		anim_pea_idle = new Animation(atlas_pea_idle, 45);
		anim_pea_break = new Animation(atlas_pea_break , 45);
		anim_pea_break->set_loop(false);//不循环
	}
	~PeaBullet() 
	{
		if (anim_pea_idle) delete anim_pea_idle;
		if (anim_pea_break) delete anim_pea_break;
	}
	
	void setBreak(bool flag) {
		is_break = flag;
	}

	bool isOver() {
		return is_break&& anim_pea_break->checkfinish();
	}

	const POINT& GetPosition() const {
		return position;
	}

	const POINT GetHeartPosition() const{
		return POINT({ position.x + img_width / 2, position.y + img_width / 2 });
	}

	void on_update(int interval)
	{
		//解决斜着移动速度是根号2倍的问题
		
		if (!is_break) {
			int delta_x = (int)(SPEED * normalize_x * interval);
			int delta_y = (int)(SPEED * normalize_y * interval);

			////到达目的地就停下
			//if (abs(delta_x) >= abs(destination.x - position.x) || abs(delta_y) >= abs(destination.y - position.y)) {
			//	position = destination;
			//}

			position.x += delta_x;
			position.y += delta_y;
		}
		//life_time -= interval;
		pea_range -= SPEED* interval;
		if (pea_range <= 0) { 
			is_break = true;
		}
		anim_pea_idle->on_update(interval);

		if (is_break) {//碎了才开始播放
			anim_pea_break->on_update(interval);
		}
	}

	void Draw()
	{
		if (!is_break) {
			anim_pea_idle->on_draw(position.x, position.y);
			//anim_pea_idle->play(position.x, position.y, delay);
		}
		else {
			anim_pea_break->on_draw(position.x, position.y);
			//anim_pea_break->play(position.x, position.y, delay);
		}
	}
	

private:
	const int img_width = 64;
	const int img_height = 64;
	const double SPEED = 1;//每毫秒的移速
	//int life_time = 3000;//生存时间3秒
	double pea_range = 600;
	
	bool is_break = false;

	POINT position = { 0 };
	POINT destination = { 0 };
	double normalize_x = 0;
	double normalize_y = 0;

	Animation* anim_pea_idle;
	Animation* anim_pea_break;
};
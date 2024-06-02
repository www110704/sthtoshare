#pragma once
#include<graphics.h>
#include"Animations.h"

extern bool is_game_started;
extern bool running;


class Button
{
public:
	Button(RECT rect,LPCTSTR path_img_idle,LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
	{
		region = rect;
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushde, path_img_pushed);
	}

	virtual ~Button() = default;


	void ProcessEvent(const ExMessage& msg)
	{
		//状态机
		switch (msg.message) {
		case WM_MOUSEMOVE://移动信号
			//当前状态时闲置且进入按钮--》变为悬停
			if (btn_status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				btn_status = Status::Hovered;
			//当前状态按下且移出按钮--》变为闲置
			if (btn_status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				btn_status = Status::Idle;
			break;
		case WM_LBUTTONDOWN://按下信号
			//在按钮内按下--》变为按下
			if (CheckCursorHit(msg.x, msg.y))
				btn_status = Status::Pushde;
			break;
		case WM_LBUTTONUP://释放信号
			//当前为按下状态--》触发点击响应函数
			if (btn_status == Status::Pushde) {
				Clicked();
				btn_status = Status::Idle;//重置状态
			}
			break;
		default:
			break;
		}
	}


	void Draw()
	{
		switch (btn_status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle);
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered);
			break;
		case Status::Pushde:
			putimage(region.left, region.top, &img_pushde);
			break;
		default:
			break;
		}
	}

protected:
	virtual void Clicked() = 0;

private:
	bool CheckCursorHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}

private:
	enum class Status
	{
		Idle = 0,
		Hovered,
		Pushde
	};

	RECT region;
	IMAGE img_idle;//闲置
	IMAGE img_hovered;//悬停
	IMAGE img_pushde;//按下
	Status btn_status = Status::Idle;
};

class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed)//初始化父类
	{}

	~StartGameButton() = default;

protected:
	void Clicked()
	{
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//播放 bgm 循环 从 0开始
	}
};

class QuitGameButton : public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed)//初始化父类
	{}

	~QuitGameButton() = default;
protected:
	void Clicked()
	{
		running = false;
	}
};
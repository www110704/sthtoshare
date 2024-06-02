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
		//״̬��
		switch (msg.message) {
		case WM_MOUSEMOVE://�ƶ��ź�
			//��ǰ״̬ʱ�����ҽ��밴ť--����Ϊ��ͣ
			if (btn_status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				btn_status = Status::Hovered;
			//��ǰ״̬�������Ƴ���ť--����Ϊ����
			if (btn_status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				btn_status = Status::Idle;
			break;
		case WM_LBUTTONDOWN://�����ź�
			//�ڰ�ť�ڰ���--����Ϊ����
			if (CheckCursorHit(msg.x, msg.y))
				btn_status = Status::Pushde;
			break;
		case WM_LBUTTONUP://�ͷ��ź�
			//��ǰΪ����״̬--�����������Ӧ����
			if (btn_status == Status::Pushde) {
				Clicked();
				btn_status = Status::Idle;//����״̬
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
	IMAGE img_idle;//����
	IMAGE img_hovered;//��ͣ
	IMAGE img_pushde;//����
	Status btn_status = Status::Idle;
};

class StartGameButton : public Button
{
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed)//��ʼ������
	{}

	~StartGameButton() = default;

protected:
	void Clicked()
	{
		is_game_started = true;
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//���� bgm ѭ�� �� 0��ʼ
	}
};

class QuitGameButton : public Button
{
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed)//��ʼ������
	{}

	~QuitGameButton() = default;
protected:
	void Clicked()
	{
		running = false;
	}
};
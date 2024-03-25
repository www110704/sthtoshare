#pragma once
#include"Animations.h"


class Player 
{
public:
	Player() 
	{
		//���ض���
		loadimage(&img_shadow, _T("res/img/shadow_player.png"));
		anim_left = new Animation(_T("res/img/player_left_%d.png"), 6, 45);
		anim_right = new Animation(_T("res/img/player_right_%d.png"), 6, 45);
	}

	~Player()
	{
		delete anim_left;
		delete anim_right;
	}

	const POINT& GetPosition() const //const�����������ã�Ҫ��֤����ֵ���ܱ��ı䣬������Ҫconst���η���ֵ
	{
		return player_pos;
	}

	void ProcessEvent(const ExMessage& msg)
	{
		//��������()
		if (msg.message == WM_KEYDOWN) {//���̰���,��ֱ�Ӽ���λ��,���ǿ���λ���ж�
			switch (msg.vkcode) {
			case VK_UP:case 0x57://W
				is_move_up = true;
				break;
			case VK_DOWN:case 0x53://S
				is_move_down = true;
				break;
			case VK_LEFT:case 0x41://A
				is_move_left = true;
				break;
			case VK_RIGHT:case 0x44://D
				is_move_right = true;
				break;
			}
		}
		else if (msg.message == WM_KEYUP) {//�����ͷ�
			switch (msg.vkcode) {
			case VK_UP:case 0x57:
				is_move_up = false;
				break;
			case VK_DOWN:case 0x53:
				is_move_down = false;
				break;
			case VK_LEFT:case 0x41:
				is_move_left = false;
				break;
			case VK_RIGHT:case 0x44:
				is_move_right = false;
				break;
			}
		}
	}

	void Move()
	{
		//���б���ƶ��ٶ��Ǹ���2��������
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double move_len = sqrt(dir_x * dir_x + dir_y * dir_y);//�ƶ��ܾ���
		if (move_len != 0) {
			double normalize_x = dir_x / move_len;//x�����ƶ��Ĺ�һ������
			double normalize_y = dir_y / move_len;//y�����ƶ��Ĺ�һ������
			player_pos.x += (int)(SPEED * normalize_x);
			player_pos.y += (int)(SPEED * normalize_y);
		}

		//�����λ�ý���У׼����ֹ����
		if (player_pos.x < 0) player_pos.x = 0;
		if (player_pos.y < 0) player_pos.y = 0;
		if (player_pos.x > windows_width - FRAME_WIDTH) player_pos.x = windows_width - FRAME_WIDTH;
		if (player_pos.y > window_height - FRAME_HEIGHT) player_pos.y = window_height - FRAME_HEIGHT;
	}

	void Draw(int delay)
	{
		//������Ӱλ��
		int pos_shadow_x = player_pos.x + (FRAME_WIDTH - SHADOW_WIDTH) / 2;
		int pos_shadow_y = player_pos.y + FRAME_HEIGHT - 8;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		static bool facing_left = false;//��Ҫ�����ĸ�������ƶ�boolֵ�жϳ���,�������಻��Ҫ
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;//==0����

		//�����泯λ�û��ƶ���
		if (facing_left)
			anim_left->play(player_pos.x, player_pos.y, delay);
		else
			anim_right->play(player_pos.x, player_pos.y, delay);
	}

public:
	const int FRAME_WIDTH = 80;//����ͨ���ӿڻ�ȡ
	const int FRAME_HEIGHT = 80;
private:
	const int SPEED = 3;
	const int SHADOW_WIDTH = 32;

	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT player_pos = {500,500};
	bool is_move_left = false;
	bool is_move_right = false;
	bool is_move_up = false;
	bool is_move_down = false;
};
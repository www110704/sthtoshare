#pragma once
//#include"Animations.h"
#include"Bullet.h"
#include"Player.h"


class Enemy {
	public:
	Enemy()
	{
		//���ض���
		loadimage(&img_shadow, _T("res/img/shadow_enemy.png"));
		anim_left = new Animation(_T("res/img/enemy_left_%d.png"), 6, 45);
		anim_right = new Animation(_T("res/img/enemy_right_%d.png"), 6, 45);
		
		RandomCreate();
		
	}

	~Enemy()
	{
		delete anim_left;
		delete anim_right;
	}

	//�������
	void RandomCreate() {
		enum class SpawnEdge//���ɱ߽�
		{
			Up = 0,
			Down,
			Left,
			Right
		};
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge) {
		case SpawnEdge::Up:
			position.x = rand() % windows_width;
			position.y = -FRAME_HEIGHT;
			break;
		case SpawnEdge::Down:
			position.x = rand() % windows_width;
			position.y = window_height;
			break;
		case SpawnEdge::Left:
			position.x = -FRAME_WIDTH;
			position.y = rand() % window_height;
			break;
		case SpawnEdge::Right:
			position.x = windows_width;
			position.y = rand() % window_height;
			break;
		default:
			break;
		}
	}

	//������ƶ�
	void Move(const Player& player) {
		const POINT& player_postion = player.GetPosition();//const���ã�player�ڲ����ɱ䣬���GetPosition()Ҳ��Ҫʱconst����
		int dir_x = player_postion.x - position.x;
		int dir_y = player_postion.y - position.y;
		double move_len = sqrt(dir_x * dir_x + dir_y * dir_y);//�ƶ��ܾ���
		if (move_len != 0) {
			double normalize_x = dir_x / move_len;//x�����ƶ��Ĺ�һ������
			double normalize_y = dir_y / move_len;//y�����ƶ��Ĺ�һ������
			position.x += (int)(SPEED * normalize_x);
			position.y += (int)(SPEED * normalize_y);
		}

		//��������ҵ�λ�ù�ϵ�жϲ����Ǹ�����
		if (dir_x < 0) facing_left = true;
		else if (dir_x > 0) facing_left = false;
	}

	//����
	void Draw(int delay) {
		//��Ӱλ��
		int pos_shadow_x = position.x + (FRAME_WIDTH - SHADOW_WIDTH) / 2;
		int pos_shadow_y = position.y + FRAME_HEIGHT - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		//�����泯λ�û��ƶ���
		if (facing_left)
			anim_left->play(position.x, position.y, delay);
		else
			anim_right->play(position.x, position.y, delay);
	}

	//��ײ(�ӵ������)
	bool CheckBulletCollision(const Bullet& bullet) {
		//���Ƿ��ھ�����
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}
	//�����޸���ײ����ı���Ϸ�Ѷ�
	bool CheckPlayerCollision(const Player& player) {
		//�������ε��ཻ���--����ײ���С��ʵ�����--�������˵����ĵ�
		POINT check_position = { position.x + FRAME_WIDTH / 2,position.y + FRAME_HEIGHT / 2 };
		bool is_overlap_x = check_position.x >= player.GetPosition().x && check_position.x <= player.GetPosition().x + player.FRAME_WIDTH;
		bool is_overlap_y = check_position.y >= player.GetPosition().y && check_position.y <= player.GetPosition().y + player.FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}

	//�ܻ�
	void Hurt()
	{
		//����Ѫ��
		alive = false;//һ����ɱ
	}

	bool CheckAlive()
	{
		return alive;
		//return healthy<=0;
	}

private:
	const int SPEED = 2;
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;
	const int SHADOW_WIDTH = 48;

	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 500,500 };
	bool facing_left = false;
	bool alive = true;
	
	//int healthy;//Ѫ��
};
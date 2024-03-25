#pragma once
//#include"Animations.h"
#include"Bullet.h"
#include"Player.h"


class Enemy {
	public:
	Enemy()
	{
		//加载动画
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

	//随机生成
	void RandomCreate() {
		enum class SpawnEdge//生成边界
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

	//向玩家移动
	void Move(const Player& player) {
		const POINT& player_postion = player.GetPosition();//const引用，player内部不可变，因此GetPosition()也需要时const函数
		int dir_x = player_postion.x - position.x;
		int dir_y = player_postion.y - position.y;
		double move_len = sqrt(dir_x * dir_x + dir_y * dir_y);//移动总距离
		if (move_len != 0) {
			double normalize_x = dir_x / move_len;//x方向移动的归一化距离
			double normalize_y = dir_y / move_len;//y方向移动的归一化距离
			position.x += (int)(SPEED * normalize_x);
			position.y += (int)(SPEED * normalize_y);
		}

		//根据与玩家的位置关系判断播放那个朝向
		if (dir_x < 0) facing_left = true;
		else if (dir_x > 0) facing_left = false;
	}

	//绘制
	void Draw(int delay) {
		//阴影位置
		int pos_shadow_x = position.x + (FRAME_WIDTH - SHADOW_WIDTH) / 2;
		int pos_shadow_y = position.y + FRAME_HEIGHT - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		//根据面朝位置绘制动画
		if (facing_left)
			anim_left->play(position.x, position.y, delay);
		else
			anim_right->play(position.x, position.y, delay);
	}

	//碰撞(子弹、玩家)
	bool CheckBulletCollision(const Bullet& bullet) {
		//点是否在矩形内
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}
	//可以修改碰撞规则改变游戏难度
	bool CheckPlayerCollision(const Player& player) {
		//两个矩形的相交检测--》碰撞面积小于实际面积--》检测敌人的中心点
		POINT check_position = { position.x + FRAME_WIDTH / 2,position.y + FRAME_HEIGHT / 2 };
		bool is_overlap_x = check_position.x >= player.GetPosition().x && check_position.x <= player.GetPosition().x + player.FRAME_WIDTH;
		bool is_overlap_y = check_position.y >= player.GetPosition().y && check_position.y <= player.GetPosition().y + player.FRAME_HEIGHT;
		return is_overlap_x && is_overlap_y;
	}

	//受击
	void Hurt()
	{
		//消减血量
		alive = false;//一击必杀
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
	
	//int healthy;//血量
};
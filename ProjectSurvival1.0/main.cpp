#include"Animations.h"
#include"Player.h"
#include"Enemy.h"
#include"Bullet.h"

int interval = 1000 / 144;//每一次循环的间隔
int Generate_interval = 100;//敌人生成间隔
int bullet_num = 4;//子弹数量
const double pi = 3.1415926;

//生成敌人
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	static int counter = 0;
	if (++counter % Generate_interval == 0) {
		enemy_list.push_back(new Enemy());
	}
}

//跟新子弹位置
void UpdateBulletPosition(std::vector<Bullet>& bullet_list,const Player& player) {
	const double RADIAL_SPEED = 0.0045;//径向波动速度/ms，距离玩家距离变化速度
	const double TANGENT_SPEED = 0.0040;//切向波动速度/ms，圆周运动快慢
	double radian_interval = 2 * pi / bullet_list.size();//子弹弧度间隔
	POINT player_position = player.GetPosition();
	double radius = 100 + 30 * sin(GetTickCount() * RADIAL_SPEED);//子弹与玩家的距离，sin是因为这个值需要波动变化（在-25~25之间）
	//遍历确定子弹位置
	for (size_t i = 0; i < bullet_list.size(); ++i) {
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;//当前子弹所在弧度值
		bullet_list[i].position.x = (player_position.x + player.FRAME_WIDTH / 2) + (int)(radius * sin(radian));
		bullet_list[i].position.y = (player_position.y + player.FRAME_HEIGHT / 2) + (int)(radius * cos(radian));
	}
}

//绘制玩家分数
void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text,_T("当前玩家分数：%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255,185,185));
	outtextxy(10, 10, text);
}

int main() {

	initgraph(windows_width, window_height);//创建游戏窗口

	mciSendString(_T("open res/mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//(打开 文件名 取名为 bgm)
	mciSendString(_T("open res/mus/hit.wav alias hit"), NULL, 0, NULL);
	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//播放 bgm 循环 从 0开始

	bool running = true;

	IMAGE img_background;
	loadimage(&img_background, _T("res/img/background.png"));

	//创建玩家
	Player player01;
	//创建敌人leibiao 
	std::vector<Enemy*> enemy_list;
	//创建子弹
	std::vector<Bullet> bullet_list(bullet_num);

	int score = 0;//得分

	ExMessage msg;

	BeginBatchDraw();

	while (running) {
		DWORD start_time = GetTickCount();

		//读取输入()--------------------------------
		while (peekmessage(&msg)) {//读取消息列表
			//处理数据()
			player01.ProcessEvent(msg);

		}


		//逻辑处理()-----------------------------------
		player01.Move();//玩家移动
		UpdateBulletPosition(bullet_list,player01);//更新子弹
		TryGenerateEnemy(enemy_list);//创建敌人
		for (auto& enemy : enemy_list) {
			enemy->Move(player01);
		}
		for (auto& enemy : enemy_list) {//检测碰撞
			if (enemy->CheckPlayerCollision(player01)) {
				MessageBox(GetHWnd(), _T("YOUUUU DIEEEE"), _T("GAME OVER"), MB_OK);
				running = false;
				break;
			}
			//检测所有子弹碰撞
			for (const auto& bullet : bullet_list) {
				if (enemy->CheckBulletCollision(bullet)) {
					enemy->Hurt();
					mciSendString(_T("play hit from 0"), NULL, 0, NULL);//播放 hit 从 0开始
				}
			}
		}
		//移出生命值归零的敌人
		for (size_t i = 0; i < enemy_list.size(); ++i) {
			Enemy* enemy = enemy_list[i];
			if (!enemy->CheckAlive()) {
				std::swap(enemy_list[i], enemy_list.back());
				enemy_list.pop_back();
				delete enemy;
				++score;
			}
		}


		//更新画面()--------------------------------------
		cleardevice();//清空画布

		putimage(0, 0, &img_background);//绘制背景
		player01.Draw(interval);//绘制玩家
		for (auto& enemy : enemy_list) //绘制敌人
			enemy->Draw(interval);

		for (auto& bullet : bullet_list) //绘制子弹
			bullet.Draw();
		DrawPlayerScore(score);//绘制分数


		FlushBatchDraw();//画面双缓冲机制，防止画面闪烁


		//设置刷新延迟，减少CPU消耗
		DWORD end_time = GetTickCount();
		DWORD delt_time = end_time - start_time;
		if (delt_time < interval) {
			Sleep(interval - delt_time);
		}

	}


	EndBatchDraw();


}
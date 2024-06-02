#include"Animations.h"
#include"Player.h"
#include"Enemy.h"
#include"Bullet.h"
#include"Button.h"
#include<time.h>



//定义全局享变量
Atlas* atlas_player_left = nullptr;
Atlas* atlas_player_right = nullptr;

Atlas* atlas_enemy_left = nullptr;
Atlas* atlas_enemy_right = nullptr;

Atlas* atlas_pea_idle = nullptr;
Atlas* atlas_pea_break = nullptr;

IMAGE img_background;
IMAGE img_menu;



int windows_width = 1280;
int window_height = 720;

bool is_game_started = false;
bool running = true;

int FPS = 144;
int interval = 1000 / FPS;//每一次循环的间隔
int Generate_interval = 300;//敌人生成间隔
int bullet_num = 4;//子弹数量
const double pi = 3.1415926;

POINT mouse_pos = {0};

//Player player01;//创建玩家---为什么不能是全局变量？？？
std::vector<Enemy*> enemy_list;			// 敌人列表
std::vector<Bullet> bullet_list(bullet_num);//创建环绕子弹
std::vector<PeaBullet*> pea_list;		//发射的子弹列表
int score = 0;//得分


//生成敌人---应该根据时间而不是帧数
void TryGenerateEnemy(int interval)
{
	static int timer = 0;
	if ((timer+= interval)>= Generate_interval) {
		enemy_list.push_back(new Enemy());
		timer = 0;
	}
}

//跟新子弹位置
void UpdateBulletPosition(std::vector<Bullet>& bullet_list,const Player& player) {
	const double RADIAL_SPEED = 0.0045;//径向波动速度/ms，距离玩家距离变化速度
	const double TANGENT_SPEED = 0.0025;//切向波动速度/ms，圆周运动快慢
	double radian_interval = 2 * pi / bullet_list.size();//子弹弧度间隔
	POINT player_position = player.GetPosition();
	double radius = 125 + 25 * sin(GetTickCount() * RADIAL_SPEED);//子弹与玩家的距离，sin是因为这个值需要波动变化（在-25~25之间）
	//遍历确定子弹位置
	for (size_t i = 0; i < bullet_list.size(); ++i) {
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;//当前子弹所在弧度值
		bullet_list[i].SetPosition((player_position.x + player.FRAME_WIDTH / 2) + (int)(radius * sin(radian)), 
			(player_position.y + player.FRAME_HEIGHT / 2) + (int)(radius * cos(radian)));
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

//加载游戏资源
void LoadResources() {
	enemy_list.reserve(100);
	pea_list.reserve(100);

	//定义全局资源
	atlas_player_left = new Atlas(_T("res/img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("res/img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("res/img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("res/img/enemy_right_%d.png"), 6);
	atlas_pea_idle = new Atlas(_T("res/img/pea.png"), 1);
	atlas_pea_break = new Atlas(_T("res/img/pea_break_%d.png"), 3);

	loadimage(&img_menu, _T("res/img/menu.png"));
	loadimage(&img_background, _T("res/img/background.png"));

	mciSendString(_T("open res/mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//(打开 文件名 取名为 bgm)
	mciSendString(_T("open res/mus/hit.wav alias hit"), NULL, 0, NULL);
	mciSendString(_T("open res/mus/pea_break.mp3 alias pea_break"), NULL, 0, NULL);
	mciSendString(_T("open res/mus/pea_shoot.mp3 alias pea_shoot"), NULL, 0, NULL);
	//mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//播放 bgm 循环 从 0开始
}


void UnloadResources() {
	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;

	for (Enemy* enemy : enemy_list)
		delete enemy;

	for (PeaBullet* pea : pea_list)
		delete pea;
}

//死亡初始化资源重新开始游戏
void initGame() {

}

int main() {
	LoadResources();
	srand((unsigned int)time(NULL));

	HWND hwnd = initgraph(windows_width, window_height, EW_SHOWCONSOLE);//创建游戏窗口
	SetWindowText(hwnd, _T("提瓦特幸存者 - 1.0"));

	//按钮定义
	RECT region_btn_start, region_btn_quit;//按按钮区域
	region_btn_start.left = (windows_width-BUTTON_WIDTH) / 2;
	region_btn_start.right = region_btn_start.left + BUTTON_WIDTH;
	region_btn_start.top = 430;
	region_btn_start.bottom = region_btn_start.top + BUTTON_HEIGHT;

	region_btn_quit.left = (windows_width - BUTTON_WIDTH) / 2;
	region_btn_quit.right = region_btn_quit.left + BUTTON_WIDTH;
	region_btn_quit.top = 550;
	region_btn_quit.bottom = region_btn_quit.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start,
		_T("res/img/ui_start_idle.png"), _T("res/img/ui_start_hovered.png"), _T("res/img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit, 
		_T("res/img/ui_quit_idle.png"), _T("res/img/ui_quit_hovered.png"), _T("res/img/ui_quit_pushed.png"));


	Player player01;//创建玩家---为什么不能是全局变量？？？
	//std::vector<Enemy*> enemy_list;			// 敌人列表
	//std::vector<Bullet> bullet_list(bullet_num);//创建子弹
	//int score = 0;//得分
	
	ExMessage msg;

	

	BeginBatchDraw();

	//需要判断是主菜单逻辑还是游戏内逻辑
	while (running) {
		DWORD start_time = GetTickCount();

		//读取输入()--------------------------------
		//DWORD read_start_time = GetTickCount();
		while (peekmessage(&msg)) {//读取消息列表
			if (msg.message == WM_MOUSEMOVE) {//跟随鼠标移动
				mouse_pos.x = msg.x;
				mouse_pos.y = msg.y;
			}
			
			//处理数据()
			if (is_game_started) {
				player01.ProcessEvent(msg);
				if (msg.message == WM_LBUTTONDOWN) {//鼠标点击产生子弹,如何控制发射间隔？
					pea_list.push_back(new PeaBullet(player01.GetPosition(), mouse_pos));
					mciSendString(_T("play pea_shoot from 0"), NULL, 0, NULL);//播放 shoot 从 0开始
				}
			}
			else {
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}
		/*DWORD read_end_time = GetTickCount();
		if (read_end_time - read_start_time > 7)
		std::cout << "read_delt_time: " << read_end_time- read_start_time << std::endl;*/

		//逻辑处理()-----------------------------------如何在结束后返回主菜单，并能够重新开始游戏？？
		if (is_game_started) {
			//DWORD update_start = GetTickCount();
			player01.Move();//玩家移动
			player01.on_update(interval);
			UpdateBulletPosition(bullet_list, player01);//更新子弹
			TryGenerateEnemy(interval);//创建敌人

			for (auto& enemy : enemy_list) {
				enemy->on_update(interval);
				enemy->Move(player01);
			}

			for (auto& pea : pea_list) {//豌豆移动
				pea->on_update(interval);
			}

			for (auto& enemy : enemy_list) {//检测碰撞
				if (enemy->CheckPlayerCollision(player01)) {
					TCHAR message[256];
					_stprintf_s(message, _T("YOUUUU DIEEEE\n最终得分：%d"), score);
					MessageBox(GetHWnd(), message, _T("GAME OVER"), MB_OK);
					is_game_started = false;//如何清空资源重新开始游戏？
					//initGame();
					//mciSendString(_T("close bgm"), NULL, 0, NULL);//停止bgm
					break;
				}

				//检测所有子弹碰撞
				for (const auto& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						enemy->Hurt();
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);//播放 hit 从 0开始
					}
				}

				//检测所有豌豆
				for (auto& pea:pea_list) {
					if (enemy->CheckPeaCollision(*pea)) {
						enemy->Hurt();
						pea->setBreak(true);
						mciSendString(_T("play pea_break from 0"), NULL, 0, NULL);//播放 hit 从 0开始
					}
				}

			}

			//移出生命值归零的敌人---移到最后然后delete然后pop_back
			for (size_t i = 0; i < enemy_list.size(); ++i) {
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive()) {
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
					++score;
					if (score >= 15) Generate_interval = 150;
					else if (score >= 50) Generate_interval = 50;
				}
			}
			
			//移出破碎且播放完成的的豌豆
			for (size_t i = 0; i < pea_list.size(); ++i) {
				PeaBullet* pea = pea_list[i];
				if (pea->isOver()) {
					std::swap(pea_list[i], pea_list.back());
					pea_list.pop_back();
					delete pea;
				}
			}
		}

		//更新画面()--------------------------------------

		cleardevice();//清空画布

		if (is_game_started) {
			putimage(0, 0, &img_background);//绘制背景
			player01.Draw();//绘制玩家
			for (auto& enemy : enemy_list) //绘制敌人
				enemy->Draw();

			for (auto& bullet : bullet_list) //绘制子弹
				bullet.Draw();

			for (auto& pea : pea_list) //绘制豌豆
				pea->Draw();

			DrawPlayerScore(score);//绘制分数

			//绘制准心
			setlinecolor(RGB(255, 0, 0));
			line(mouse_pos.x - 5, mouse_pos.y, mouse_pos.x + 5, mouse_pos.y);
			line(mouse_pos.x, mouse_pos.y - 5, mouse_pos.x, mouse_pos.y + 5);
			circle(mouse_pos.x, mouse_pos.y, 10);
		}
		else {
			putimage(0, 0, &img_menu);//绘制开始菜单
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}

		FlushBatchDraw();//画面双缓冲机制，防止画面闪烁

		//设置刷新延迟，减少CPU消耗
		DWORD end_time = GetTickCount();
		DWORD delt_time = end_time - start_time;
		if(delt_time>5)	std::cout << "frame_delt_time: " << delt_time << std::endl;
		if (delt_time < interval) {
			Sleep(interval - delt_time);
		}
	}
	
	EndBatchDraw();

	UnloadResources();
	return 0;
}
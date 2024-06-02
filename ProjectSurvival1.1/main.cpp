#include"Animations.h"
#include"Player.h"
#include"Enemy.h"
#include"Bullet.h"
#include"Button.h"
#include<time.h>



//����ȫ�������
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
int interval = 1000 / FPS;//ÿһ��ѭ���ļ��
int Generate_interval = 300;//�������ɼ��
int bullet_num = 4;//�ӵ�����
const double pi = 3.1415926;

POINT mouse_pos = {0};

//Player player01;//�������---Ϊʲô������ȫ�ֱ���������
std::vector<Enemy*> enemy_list;			// �����б�
std::vector<Bullet> bullet_list(bullet_num);//���������ӵ�
std::vector<PeaBullet*> pea_list;		//������ӵ��б�
int score = 0;//�÷�


//���ɵ���---Ӧ�ø���ʱ�������֡��
void TryGenerateEnemy(int interval)
{
	static int timer = 0;
	if ((timer+= interval)>= Generate_interval) {
		enemy_list.push_back(new Enemy());
		timer = 0;
	}
}

//�����ӵ�λ��
void UpdateBulletPosition(std::vector<Bullet>& bullet_list,const Player& player) {
	const double RADIAL_SPEED = 0.0045;//���򲨶��ٶ�/ms��������Ҿ���仯�ٶ�
	const double TANGENT_SPEED = 0.0025;//���򲨶��ٶ�/ms��Բ���˶�����
	double radian_interval = 2 * pi / bullet_list.size();//�ӵ����ȼ��
	POINT player_position = player.GetPosition();
	double radius = 125 + 25 * sin(GetTickCount() * RADIAL_SPEED);//�ӵ�����ҵľ��룬sin����Ϊ���ֵ��Ҫ�����仯����-25~25֮�䣩
	//����ȷ���ӵ�λ��
	for (size_t i = 0; i < bullet_list.size(); ++i) {
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;//��ǰ�ӵ����ڻ���ֵ
		bullet_list[i].SetPosition((player_position.x + player.FRAME_WIDTH / 2) + (int)(radius * sin(radian)), 
			(player_position.y + player.FRAME_HEIGHT / 2) + (int)(radius * cos(radian)));
	}
}

//������ҷ���
void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text,_T("��ǰ��ҷ�����%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255,185,185));
	outtextxy(10, 10, text);
}

//������Ϸ��Դ
void LoadResources() {
	enemy_list.reserve(100);
	pea_list.reserve(100);

	//����ȫ����Դ
	atlas_player_left = new Atlas(_T("res/img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("res/img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("res/img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("res/img/enemy_right_%d.png"), 6);
	atlas_pea_idle = new Atlas(_T("res/img/pea.png"), 1);
	atlas_pea_break = new Atlas(_T("res/img/pea_break_%d.png"), 3);

	loadimage(&img_menu, _T("res/img/menu.png"));
	loadimage(&img_background, _T("res/img/background.png"));

	mciSendString(_T("open res/mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//(�� �ļ��� ȡ��Ϊ bgm)
	mciSendString(_T("open res/mus/hit.wav alias hit"), NULL, 0, NULL);
	mciSendString(_T("open res/mus/pea_break.mp3 alias pea_break"), NULL, 0, NULL);
	mciSendString(_T("open res/mus/pea_shoot.mp3 alias pea_shoot"), NULL, 0, NULL);
	//mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//���� bgm ѭ�� �� 0��ʼ
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

//������ʼ����Դ���¿�ʼ��Ϸ
void initGame() {

}

int main() {
	LoadResources();
	srand((unsigned int)time(NULL));

	HWND hwnd = initgraph(windows_width, window_height, EW_SHOWCONSOLE);//������Ϸ����
	SetWindowText(hwnd, _T("�������Ҵ��� - 1.0"));

	//��ť����
	RECT region_btn_start, region_btn_quit;//����ť����
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


	Player player01;//�������---Ϊʲô������ȫ�ֱ���������
	//std::vector<Enemy*> enemy_list;			// �����б�
	//std::vector<Bullet> bullet_list(bullet_num);//�����ӵ�
	//int score = 0;//�÷�
	
	ExMessage msg;

	

	BeginBatchDraw();

	//��Ҫ�ж������˵��߼�������Ϸ���߼�
	while (running) {
		DWORD start_time = GetTickCount();

		//��ȡ����()--------------------------------
		//DWORD read_start_time = GetTickCount();
		while (peekmessage(&msg)) {//��ȡ��Ϣ�б�
			if (msg.message == WM_MOUSEMOVE) {//��������ƶ�
				mouse_pos.x = msg.x;
				mouse_pos.y = msg.y;
			}
			
			//��������()
			if (is_game_started) {
				player01.ProcessEvent(msg);
				if (msg.message == WM_LBUTTONDOWN) {//����������ӵ�,��ο��Ʒ�������
					pea_list.push_back(new PeaBullet(player01.GetPosition(), mouse_pos));
					mciSendString(_T("play pea_shoot from 0"), NULL, 0, NULL);//���� shoot �� 0��ʼ
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

		//�߼�����()-----------------------------------����ڽ����󷵻����˵������ܹ����¿�ʼ��Ϸ����
		if (is_game_started) {
			//DWORD update_start = GetTickCount();
			player01.Move();//����ƶ�
			player01.on_update(interval);
			UpdateBulletPosition(bullet_list, player01);//�����ӵ�
			TryGenerateEnemy(interval);//��������

			for (auto& enemy : enemy_list) {
				enemy->on_update(interval);
				enemy->Move(player01);
			}

			for (auto& pea : pea_list) {//�㶹�ƶ�
				pea->on_update(interval);
			}

			for (auto& enemy : enemy_list) {//�����ײ
				if (enemy->CheckPlayerCollision(player01)) {
					TCHAR message[256];
					_stprintf_s(message, _T("YOUUUU DIEEEE\n���յ÷֣�%d"), score);
					MessageBox(GetHWnd(), message, _T("GAME OVER"), MB_OK);
					is_game_started = false;//��������Դ���¿�ʼ��Ϸ��
					//initGame();
					//mciSendString(_T("close bgm"), NULL, 0, NULL);//ֹͣbgm
					break;
				}

				//��������ӵ���ײ
				for (const auto& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						enemy->Hurt();
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);//���� hit �� 0��ʼ
					}
				}

				//��������㶹
				for (auto& pea:pea_list) {
					if (enemy->CheckPeaCollision(*pea)) {
						enemy->Hurt();
						pea->setBreak(true);
						mciSendString(_T("play pea_break from 0"), NULL, 0, NULL);//���� hit �� 0��ʼ
					}
				}

			}

			//�Ƴ�����ֵ����ĵ���---�Ƶ����Ȼ��deleteȻ��pop_back
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
			
			//�Ƴ������Ҳ�����ɵĵ��㶹
			for (size_t i = 0; i < pea_list.size(); ++i) {
				PeaBullet* pea = pea_list[i];
				if (pea->isOver()) {
					std::swap(pea_list[i], pea_list.back());
					pea_list.pop_back();
					delete pea;
				}
			}
		}

		//���»���()--------------------------------------

		cleardevice();//��ջ���

		if (is_game_started) {
			putimage(0, 0, &img_background);//���Ʊ���
			player01.Draw();//�������
			for (auto& enemy : enemy_list) //���Ƶ���
				enemy->Draw();

			for (auto& bullet : bullet_list) //�����ӵ�
				bullet.Draw();

			for (auto& pea : pea_list) //�����㶹
				pea->Draw();

			DrawPlayerScore(score);//���Ʒ���

			//����׼��
			setlinecolor(RGB(255, 0, 0));
			line(mouse_pos.x - 5, mouse_pos.y, mouse_pos.x + 5, mouse_pos.y);
			line(mouse_pos.x, mouse_pos.y - 5, mouse_pos.x, mouse_pos.y + 5);
			circle(mouse_pos.x, mouse_pos.y, 10);
		}
		else {
			putimage(0, 0, &img_menu);//���ƿ�ʼ�˵�
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}

		FlushBatchDraw();//����˫������ƣ���ֹ������˸

		//����ˢ���ӳ٣�����CPU����
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
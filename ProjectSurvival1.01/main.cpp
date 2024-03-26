#include"Animations.h"
#include"Player.h"
#include"Enemy.h"
#include"Bullet.h"
#include"Button.h"

int interval = 1000 / 144;//ÿһ��ѭ���ļ��
int Generate_interval = 100;//�������ɼ��
int bullet_num = 4;//�ӵ�����
const double pi = 3.1415926;


//���ɵ���
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	static int counter = 0;
	if (++counter % Generate_interval == 0) {
		enemy_list.push_back(new Enemy());
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
		bullet_list[i].position.x = (player_position.x + player.FRAME_WIDTH / 2) + (int)(radius * sin(radian));
		bullet_list[i].position.y = (player_position.y + player.FRAME_HEIGHT / 2) + (int)(radius * cos(radian));
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

//������ʼ����Դ���¿�ʼ��Ϸ
void initGame() {

}

int main() {

	initgraph(windows_width, window_height);//������Ϸ����

	mciSendString(_T("open res/mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//(�� �ļ��� ȡ��Ϊ bgm)
	mciSendString(_T("open res/mus/hit.wav alias hit"), NULL, 0, NULL);
	//mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//���� bgm ѭ�� �� 0��ʼ

	//����ȫ����Դ
	atlas_player_left = new Atlas(_T("res/img/player_left_%d.png"),6);
	atlas_player_right = new Atlas(_T("res/img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("res/img/enemy_left_%d.png"), 6);
	atlas_enemy_right  = new Atlas(_T("res/img/enemy_right_%d.png"), 6);

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


	IMAGE img_background;
	IMAGE img_menu;
	Player player01;//�������
	std::vector<Enemy*> enemy_list;//�����б� 
	std::vector<Bullet> bullet_list(bullet_num);//�����ӵ�
	int score = 0;//�÷�
	ExMessage msg;

	loadimage(&img_menu, _T("res/img/menu.png"));
	loadimage(&img_background, _T("res/img/background.png"));

	BeginBatchDraw();

	//��Ҫ�ж������˵��߼�������Ϸ���߼�
	while (running) {
		DWORD start_time = GetTickCount();

		//��ȡ����()--------------------------------
		while (peekmessage(&msg)) {//��ȡ��Ϣ�б�
			//��������()
			if (is_game_started) 
				player01.ProcessEvent(msg);
			else {
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}

		//�߼�����()-----------------------------------����ڽ����󷵻����˵������ܹ����¿�ʼ��Ϸ����
		if (is_game_started) {
			player01.Move();//����ƶ�
			UpdateBulletPosition(bullet_list, player01);//�����ӵ�
			TryGenerateEnemy(enemy_list);//��������
			for (auto& enemy : enemy_list) {
				enemy->Move(player01);
			}
			for (auto& enemy : enemy_list) {//�����ײ
				if (enemy->CheckPlayerCollision(player01)) {
					TCHAR message[256];
					_stprintf_s(message, _T("YOUUUU DIEEEE\n���յ÷֣�%d"), score);
					MessageBox(GetHWnd(), message, _T("GAME OVER"), MB_OK);
					is_game_started = false;//��������Դ���¿�ʼ��Ϸ��
					//initGame();
					mciSendString(_T("close bgm"), NULL, 0, NULL);//ֹͣbgm
					break;
				}
				//��������ӵ���ײ
				for (const auto& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						enemy->Hurt();
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);//���� hit �� 0��ʼ
					}
				}
			}
			//�Ƴ�����ֵ����ĵ���
			for (size_t i = 0; i < enemy_list.size(); ++i) {
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive()) {
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
					++score;
				}
			}
		}
		

		//���»���()--------------------------------------
		cleardevice();//��ջ���

		if (is_game_started) {
			putimage(0, 0, &img_background);//���Ʊ���
			player01.Draw(interval);//�������
			for (auto& enemy : enemy_list) //���Ƶ���
				enemy->Draw(interval);

			for (auto& bullet : bullet_list) //�����ӵ�
				bullet.Draw();
			DrawPlayerScore(score);//���Ʒ���
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
		if (delt_time < interval) {
			Sleep(interval - delt_time);
		}
	}
	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;


	EndBatchDraw();

	return 0;
}
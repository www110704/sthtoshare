#include"Animations.h"
#include"Player.h"
#include"Enemy.h"
#include"Bullet.h"

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
	const double TANGENT_SPEED = 0.0040;//���򲨶��ٶ�/ms��Բ���˶�����
	double radian_interval = 2 * pi / bullet_list.size();//�ӵ����ȼ��
	POINT player_position = player.GetPosition();
	double radius = 100 + 30 * sin(GetTickCount() * RADIAL_SPEED);//�ӵ�����ҵľ��룬sin����Ϊ���ֵ��Ҫ�����仯����-25~25֮�䣩
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

int main() {

	initgraph(windows_width, window_height);//������Ϸ����

	mciSendString(_T("open res/mus/bgm.mp3 alias bgm"), NULL, 0, NULL);//(�� �ļ��� ȡ��Ϊ bgm)
	mciSendString(_T("open res/mus/hit.wav alias hit"), NULL, 0, NULL);
	mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);//���� bgm ѭ�� �� 0��ʼ

	bool running = true;

	IMAGE img_background;
	loadimage(&img_background, _T("res/img/background.png"));

	//�������
	Player player01;
	//��������leibiao 
	std::vector<Enemy*> enemy_list;
	//�����ӵ�
	std::vector<Bullet> bullet_list(bullet_num);

	int score = 0;//�÷�

	ExMessage msg;

	BeginBatchDraw();

	while (running) {
		DWORD start_time = GetTickCount();

		//��ȡ����()--------------------------------
		while (peekmessage(&msg)) {//��ȡ��Ϣ�б�
			//��������()
			player01.ProcessEvent(msg);

		}


		//�߼�����()-----------------------------------
		player01.Move();//����ƶ�
		UpdateBulletPosition(bullet_list,player01);//�����ӵ�
		TryGenerateEnemy(enemy_list);//��������
		for (auto& enemy : enemy_list) {
			enemy->Move(player01);
		}
		for (auto& enemy : enemy_list) {//�����ײ
			if (enemy->CheckPlayerCollision(player01)) {
				MessageBox(GetHWnd(), _T("YOUUUU DIEEEE"), _T("GAME OVER"), MB_OK);
				running = false;
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


		//���»���()--------------------------------------
		cleardevice();//��ջ���

		putimage(0, 0, &img_background);//���Ʊ���
		player01.Draw(interval);//�������
		for (auto& enemy : enemy_list) //���Ƶ���
			enemy->Draw(interval);

		for (auto& bullet : bullet_list) //�����ӵ�
			bullet.Draw();
		DrawPlayerScore(score);//���Ʒ���


		FlushBatchDraw();//����˫������ƣ���ֹ������˸


		//����ˢ���ӳ٣�����CPU����
		DWORD end_time = GetTickCount();
		DWORD delt_time = end_time - start_time;
		if (delt_time < interval) {
			Sleep(interval - delt_time);
		}

	}


	EndBatchDraw();


}
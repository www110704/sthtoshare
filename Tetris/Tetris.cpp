#include "Tetris.h"
#include<time.h>
#include<stdlib.h>

#define RECORD_FILE "record.txt"

const int SPEED_SLOW = 1000;
const int SPEED_NORMAL = 500;
const int SPEED_FAST = 300;
const int SPEED_DROP = 50;


Tetris::Tetris(int rows, int cols, int edge_left, int edge_top, int blockSize)
	:rows(rows),cols(cols),leftMargin(edge_left),topMargin(edge_top),blockSize(blockSize)
{
	//to do:
	grids = vector<vector<int>>(rows, vector<int>(cols, -1));//-1��ʾû�з��鲻����Ⱦ

}


void Tetris::init()
{
	//���ű�������
	//mciSendString("play res/bg.mp3", 0, 0, 0);

	delayTime = SPEED_NORMAL;//�ɱ��ٶ�

	//�����������
	srand(time(NULL));

	//������Ϸ����
	initgraph(1000,1000);

	//������ʾ����
	loadimage(&imgBG, "res/gameInterface.png");
	loadimage(&imgOver, "res/over.png");
	loadimage(&imgWin, "res/win.png");
	
	//��ʼ����Ϸ���������
	grids = vector<vector<int>>(rows, vector<int>(cols, -1));

	b_update = false;
	gameOver = false;

	//��ʼ������
	score = 0;
	curlevel = 1;
	lineCount = 0;

	//��ȡ��߷�
	ifstream file(RECORD_FILE);
	if (!file.is_open()) {
		cout << RECORD_FILE << "fail to open!" << endl;
		HighestScore = 0;
	}
	else {
		file >> HighestScore;
	}
	file.close();
}

void Tetris::start()
{
	//to do:
	init();

	// ���������Ԥ�淽��
	nextBlock = new Block();
	curBlock = nextBlock;
	nextBlock = new Block();


	int timer = 0;
	while (true) {//������Ϸ������ѭ��������
		//�����û�����
		keyEvent();

		//����ÿ����Ⱦ�Ĺ̶���ʱ(���Զ�����ļ��),����CPU����
		timer += getDelay();
		//cout << "timer: " << timer << endl;
		if (timer > delayTime) {

			b_update = true;

			drop();

			//���ü�ʱ
			timer = 0;
		}

		//�û������ʱ���򶼻�������Ⱦ����
		if (b_update) {
			//��Ⱦ����
			updateWindow();
			
			//������Ϸ����
			clearLine();

			b_update = false;
		}

		if (gameOver) {
			//�������
			saveScore();

			//��������
			diplayOver();

			system("pause");
		}
	}
}


/*���·��򰴼����᷵�������ַ�
�ڼ����а��¡��ϼ�����key1�᷵��key=224��key2=72��

�¼��� key1=224��key2=80��

����� key1=224��key2=75��

�Ҽ��� key1=224��key2=77��
*/
void Tetris::keyEvent()
{
	unsigned char ch;
	//�����ƶ�ƫ����
	int dx = 0;
	if (_kbhit()) {//�а�������
		ch = _getch();//��ȡ��������

		
		if (ch == 224) {
			ch = _getch();
			switch (ch) {
			case 72: //���Ͼ�����ת����
				b_rotate = true;
				break;
			case 80: //����Ϊ�����½�,�����ٵ�
				delayTime = SPEED_DROP;
				break;
			case 75://����
				dx = -1;
				break;
			case 77://����
				dx = 1;
				break;
			default:
				break;
			}
		}
	}
	
	if (b_rotate) {
		rotate();
		b_update = true;
	}

	if (dx != 0) {
		move(dx);
		b_update = true;
	}

}

void Tetris::updateWindow()
{
	putimage(0, 0, &imgBG);//���Ʊ���ͼƬ

	IMAGE** imgs = Block::getImgs();
	BeginBatchDraw();//��ʼ���ƣ�һ���Ի�����ɣ���ֹ��˸
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			if (grids[i][j] == -1) continue;//-1����գ���������ͬ��ɫ

			//�з�����̻���Ⱦ
			int x = j * blockSize + leftMargin;
			int y = i * blockSize + topMargin;

			putimage(x, y, imgs[grids[i][j]]);
		}
	}

	////���Է���
	//Block b;
	//b.draw(leftMargin, topMargin, blockSize);

	curBlock->draw(leftMargin, topMargin, blockSize);
	nextBlock->draw(leftMargin+ blockSize*11, topMargin, blockSize);//����Ԥ�������Ͻ�

	//�½����ײ�������1.vector���� 2.��Ϸ����grids��ֱ�ӱ��

	drawScore();//���Ʒ���

	EndBatchDraw();
}

//�������ε��øú�����ʱ��������һ�ε��øú�������0
int Tetris::getDelay()
{
	static unsigned long long lastTime = 0;
	unsigned long long curTime = GetTickCount();
	if (lastTime == 0) {
		lastTime = curTime;
		return 0;
	}
	else {
		int ret = curTime - lastTime;
		lastTime = curTime;
		return ret;
	}
}

//�½����ܣ������������½�
void Tetris::drop()
{
	
	bakBlock = *curBlock;//��ֵ���캯��
	curBlock->drop();

	//�����½���Χ������ÿ��С�����λ��
	if (!curBlock->blocksInGrids(grids)) {
		//cur�Ѿ��Ƿ������᷽����һ������
		bakBlock.solidify(grids);
		delete curBlock;
		curBlock = nextBlock;
		nextBlock = new Block();
		
		//�����Ϸ�Ƿ����
		checkOver();
	}
	delayTime = SPEED_NORMAL;
}

void Tetris::move(int offset)
{
	bakBlock = *curBlock;//�������ƶ�

	curBlock->move(offset);

	if (!curBlock->blocksInGrids(grids)) {//���緽�黹ԭ
		*curBlock = bakBlock;
	}

}

void Tetris::rotate()
{
	if (curBlock->getBlockType() == 6) return;//�����β���

	bakBlock = *curBlock;//�������ж�

	curBlock->rotate();

	if (!curBlock->blocksInGrids(grids)) {//���緽�黹ԭ
		*curBlock = bakBlock;
	}
	b_rotate = false;
}

//������������ɨ�裬��������ȥ�����������
void Tetris::clearLine()
{
	int lines = 0;//ͬʱ����������
	int k = rows - 1;//�洢���ݵ�����
	for (int i = rows - 1; i >= 0; --i) {
		//����i�У���������i��k��--������k����
		int count = 0;
		for (int j = 0; j < cols; ++j) {
			if (grids[i][j]!=-1) {
				count++;
			}
			grids[k][j] = grids[i][j];
		}
		if (count < cols) {
			k--;
		}
		else {
			++lines;
		}
	}

	if (lines > 0) {
		//����÷�
		int addScore[4] = { 10,30,60,100 };
		score += addScore[lines - 1];

		//������Ч
		//mciSendString("play res/xiaochu1.mp3",0,0,0);
		b_update = true;

		//ÿ100��level+1
		curlevel += score / 100;
		lineCount += lines;
	}
}

void Tetris::drawScore()
{
	char scoreText[32];
	char levelText[32];
	char linesText[32];
	char HighestScoreText[32];

	sprintf_s(scoreText, sizeof(scoreText), "%d", score);
	sprintf_s(levelText, sizeof(levelText), "%d", curlevel);
	sprintf_s(linesText, sizeof(linesText), "%d", lineCount);
	sprintf_s(HighestScoreText, sizeof(HighestScoreText), "%d", HighestScore);

	if (score > HighestScore) {//��¼��߷�
		sprintf_s(HighestScoreText, sizeof(HighestScoreText), scoreText);
		HighestScore = score;
	}
	/*switch (curlevel) {
	case 1: sprintf_s(speedText, sizeof(speedText), "SLOW");
		break;
	case 2: sprintf_s(speedText, sizeof(speedText), "NORMAL");
		break;
	default: sprintf_s(speedText, sizeof(speedText), "FAST");
		break;
	}*/
	
	//�����������
	LOGFONT f;
	gettextstyle(&f);//��ȡ��ǰ����
	f.lfHeight = 45;
	f.lfWidth = 25;
	f.lfQuality = ANTIALIASED_QUALITY;//�����
	setcolor(RGB(0, 200, 0));//������ɫ
	setbkmode(TRANSPARENT);//����͸��

	strcpy_s(f.lfFaceName, sizeof(f.lfFaceName), _T("Segoe UI Black"));
	settextstyle(&f);

	outtextxy(700, 780, scoreText);
	outtextxy(700, 855, HighestScoreText);
	outtextxy(90, 770, levelText);
	outtextxy(90, 840, linesText);

}

void Tetris::checkOver()
{
	gameOver = (curBlock->blocksInGrids(grids)==false);

}

void Tetris::saveScore()
{
	if (score > HighestScore) {//��¼��߷�
		ofstream file(RECORD_FILE, ios::out);
		if (!file.is_open()) {
			cout << RECORD_FILE << "fail to open!" << endl;
		}
		else {
			//��д��߷�
			file << score;
		}
		file.close();
	}
}

void Tetris::diplayOver()
{
	//��������
	mciSendString("stop res/bg.mp3",0,0,0);
	if (curlevel > 10) {
		//ͨ��
		putimage(262,420,&imgWin);
		//mciSendString("play res/win.mp3", 0, 0, 0);

	}
	else {
		//ʧ��
		putimage(262, 420, &imgOver);
		//mciSendString("play res/over.mp3", 0, 0, 0);
	}
}


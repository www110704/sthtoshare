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
	grids = vector<vector<int>>(rows, vector<int>(cols, -1));//-1表示没有方块不用渲染

}


void Tetris::init()
{
	//播放背景音乐
	//mciSendString("play res/bg.mp3", 0, 0, 0);

	delayTime = SPEED_NORMAL;//可变速度

	//配置随机种子
	srand(time(NULL));

	//创建游戏窗口
	initgraph(1000,1000);

	//载入显示背景
	loadimage(&imgBG, "res/gameInterface.png");
	loadimage(&imgOver, "res/over.png");
	loadimage(&imgWin, "res/win.png");
	
	//初始化游戏区域的数据
	grids = vector<vector<int>>(rows, vector<int>(cols, -1));

	b_update = false;
	gameOver = false;

	//初始化分数
	score = 0;
	curlevel = 1;
	lineCount = 0;

	//读取最高分
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

	// 创建方块和预告方块
	nextBlock = new Block();
	curBlock = nextBlock;
	nextBlock = new Block();


	int timer = 0;
	while (true) {//所有游戏都有死循环？？？
		//接收用户输入
		keyEvent();

		//设置每次渲染的固定延时(即自动下落的间隔),减少CPU运算
		timer += getDelay();
		//cout << "timer: " << timer << endl;
		if (timer > delayTime) {

			b_update = true;

			drop();

			//重置计时
			timer = 0;
		}

		//用户输入或到时间则都会立即渲染更新
		if (b_update) {
			//渲染画面
			updateWindow();
			
			//更新游戏数据
			clearLine();

			b_update = false;
		}

		if (gameOver) {
			//保存分数
			saveScore();

			//结束画面
			diplayOver();

			system("pause");
		}
	}
}


/*按下方向按键，会返回两个字符
在键盘中按下“上键”后，key1会返回key=224，key2=72；

下键： key1=224，key2=80；

左键： key1=224，key2=75；

右键： key1=224，key2=77；
*/
void Tetris::keyEvent()
{
	unsigned char ch;
	//左右移动偏移量
	int dx = 0;
	if (_kbhit()) {//有按键输入
		ch = _getch();//读取按键输入

		
		if (ch == 224) {
			ch = _getch();
			switch (ch) {
			case 72: //向上就是旋转变形
				b_rotate = true;
				break;
			case 80: //向下为快速下降,即快速档
				delayTime = SPEED_DROP;
				break;
			case 75://向左
				dx = -1;
				break;
			case 77://向右
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
	putimage(0, 0, &imgBG);//绘制背景图片

	IMAGE** imgs = Block::getImgs();
	BeginBatchDraw();//开始绘制，一次性绘制完成，防止闪烁
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			if (grids[i][j] == -1) continue;//-1代表空，其他代表不同颜色

			//有方块则固化渲染
			int x = j * blockSize + leftMargin;
			int y = i * blockSize + topMargin;

			putimage(x, y, imgs[grids[i][j]]);
		}
	}

	////测试方块
	//Block b;
	//b.draw(leftMargin, topMargin, blockSize);

	curBlock->draw(leftMargin, topMargin, blockSize);
	nextBlock->draw(leftMargin+ blockSize*11, topMargin, blockSize);//方块预告在右上角

	//下降到底部保留：1.vector数组 2.游戏网格grids上直接标记

	drawScore();//绘制分数

	EndBatchDraw();
}

//返回两次调用该函数的时间间隔，第一次调用该函数返回0
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

//下降功能；方块自生的下降
void Tetris::drop()
{
	
	bakBlock = *curBlock;//赋值构造函数
	curBlock->drop();

	//限制下降范围，遍历每个小方块的位置
	if (!curBlock->blocksInGrids(grids)) {
		//cur已经非法，冻结方块上一个方块
		bakBlock.solidify(grids);
		delete curBlock;
		curBlock = nextBlock;
		nextBlock = new Block();
		
		//检查游戏是否结束
		checkOver();
	}
	delayTime = SPEED_NORMAL;
}

void Tetris::move(int offset)
{
	bakBlock = *curBlock;//备份再移动

	curBlock->move(offset);

	if (!curBlock->blocksInGrids(grids)) {//出界方块还原
		*curBlock = bakBlock;
	}

}

void Tetris::rotate()
{
	if (curBlock->getBlockType() == 6) return;//田字形不动

	bakBlock = *curBlock;//备份再行动

	curBlock->rotate();

	if (!curBlock->blocksInGrids(grids)) {//出界方块还原
		*curBlock = bakBlock;
	}
	b_rotate = false;
}

//从下往上逐行扫描，若满则消去，不满则存下
void Tetris::clearLine()
{
	int lines = 0;//同时消除的行数
	int k = rows - 1;//存储数据的行数
	for (int i = rows - 1; i >= 0; --i) {
		//检查第i行，若不满则i和k都--，满则k不变
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
		//计算得分
		int addScore[4] = { 10,30,60,100 };
		score += addScore[lines - 1];

		//播放音效
		//mciSendString("play res/xiaochu1.mp3",0,0,0);
		b_update = true;

		//每100分level+1
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

	if (score > HighestScore) {//记录最高分
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
	
	//设置字体变量
	LOGFONT f;
	gettextstyle(&f);//获取当前字体
	f.lfHeight = 45;
	f.lfWidth = 25;
	f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿
	setcolor(RGB(0, 200, 0));//字体颜色
	setbkmode(TRANSPARENT);//字体透明

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
	if (score > HighestScore) {//记录最高分
		ofstream file(RECORD_FILE, ios::out);
		if (!file.is_open()) {
			cout << RECORD_FILE << "fail to open!" << endl;
		}
		else {
			//改写最高分
			file << score;
		}
		file.close();
	}
}

void Tetris::diplayOver()
{
	//背景音乐
	mciSendString("stop res/bg.mp3",0,0,0);
	if (curlevel > 10) {
		//通关
		putimage(262,420,&imgWin);
		//mciSendString("play res/win.mp3", 0, 0, 0);

	}
	else {
		//失败
		putimage(262, 420, &imgOver);
		//mciSendString("play res/over.mp3", 0, 0, 0);
	}
}


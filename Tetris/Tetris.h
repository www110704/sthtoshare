#pragma once
#include<vector>
#include<graphics.h>
#include"Block.h"
#include<iostream>
#include<conio.h>//按键输入识别
#include<fstream>
#include<mmsystem.h>//播放音乐文件，需要加载winmm.lib库文件
#pragma comment(lib,"winmm.lib")

using namespace std;

class Tetris
{
public:
	//判断不同图片素材的边界，确定游戏的下落区域，并根据区域大小决定合适的方块大小
	Tetris(int rows,int cols,int edge_left,int edge_top,int blockSize);
	void init();
	void start();

	

private:
	void keyEvent();//用户输入
	void updateWindow();//渲染游戏界面

	int getDelay();//返回两次调用该函数的时间间隔，第一次调用该函数返回0
	void drop();
	void move(int offset);
	void rotate();
	void clearLine();
	void drawScore();
	void checkOver();
	void saveScore();
	void diplayOver();
	
private:
	int delayTime;
	bool b_update;//是否渲染
	bool b_rotate;//是否旋转
	bool gameOver;

	vector<vector<int>> grids;//整体网格,-1表示没有方块，0~6表示7种方块
	int rows, cols;
	int leftMargin, topMargin;
	int blockSize;
	IMAGE imgBG;//界面背景图
	IMAGE imgOver;
	IMAGE imgWin;

	Block* curBlock;//当前方块
	Block* nextBlock;//预告下一个方块
	Block bakBlock;//备用方块，指向当前方块的上一个位置

	int score;//当前分数
	int curlevel;//关卡
	int lineCount;//消除行数
	int HighestScore;//历史最高分
};


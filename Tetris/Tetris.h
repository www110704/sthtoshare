#pragma once
#include<vector>
#include<graphics.h>
#include"Block.h"
#include<iostream>
#include<conio.h>//��������ʶ��
#include<fstream>
#include<mmsystem.h>//���������ļ�����Ҫ����winmm.lib���ļ�
#pragma comment(lib,"winmm.lib")

using namespace std;

class Tetris
{
public:
	//�жϲ�ͬͼƬ�زĵı߽磬ȷ����Ϸ���������򣬲����������С�������ʵķ����С
	Tetris(int rows,int cols,int edge_left,int edge_top,int blockSize);
	void init();
	void start();

	

private:
	void keyEvent();//�û�����
	void updateWindow();//��Ⱦ��Ϸ����

	int getDelay();//�������ε��øú�����ʱ��������һ�ε��øú�������0
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
	bool b_update;//�Ƿ���Ⱦ
	bool b_rotate;//�Ƿ���ת
	bool gameOver;

	vector<vector<int>> grids;//��������,-1��ʾû�з��飬0~6��ʾ7�ַ���
	int rows, cols;
	int leftMargin, topMargin;
	int blockSize;
	IMAGE imgBG;//���汳��ͼ
	IMAGE imgOver;
	IMAGE imgWin;

	Block* curBlock;//��ǰ����
	Block* nextBlock;//Ԥ����һ������
	Block bakBlock;//���÷��飬ָ��ǰ�������һ��λ��

	int score;//��ǰ����
	int curlevel;//�ؿ�
	int lineCount;//��������
	int HighestScore;//��ʷ��߷�
};


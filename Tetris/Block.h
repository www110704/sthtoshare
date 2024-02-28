#pragma once
//��װeasyxͼ�ο�
#include<graphics.h>
#include<vector>

using namespace std;

struct Point {
	int row;
	int col;
};

class Block
{
public://
	Block();
	Block& operator=(const Block& other);
	void drop();
	void move(int offset);
	void rotate();
	void draw(int edge_left,int edge_top,int size);
	static IMAGE** getImgs();//��ȡͼƬ����
	//Point* getSmallBlocks() { return smallBlocks; }//�����ж�С�����Ƿ����
	bool blocksInGrids(const vector<vector<int>>& grids);
	void solidify(vector<vector<int>>& grids);
	int getBlockType() { return blockType; }

private:
	int blockType;//��������
	Point smallBlocks[4];//��ǰÿ��С�����λ�ã�����������£�

	IMAGE* img;//EasyXͼƬ����ָ��

	static IMAGE* imgs[7];//��̬ͼƬָ������
	static int imgSize;//ÿ��С����ĳߴ�
};


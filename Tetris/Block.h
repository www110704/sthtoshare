#pragma once
//安装easyx图形库
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
	static IMAGE** getImgs();//获取图片数组
	//Point* getSmallBlocks() { return smallBlocks; }//用于判断小方块是否出界
	bool blocksInGrids(const vector<vector<int>>& grids);
	void solidify(vector<vector<int>>& grids);
	int getBlockType() { return blockType; }

private:
	int blockType;//方块类型
	Point smallBlocks[4];//当前每个小方块的位置，用于下落更新？

	IMAGE* img;//EasyX图片变量指针

	static IMAGE* imgs[7];//静态图片指针数组
	static int imgSize;//每个小方块的尺寸
};


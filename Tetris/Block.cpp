#include "Block.h"
#include<stdlib.h>

IMAGE* Block::imgs[7] = { nullptr, };
int Block::imgSize = 40;

/*初始化方块的图像
* 载入颜色素材图象，分割并存入图像指针数组中
* 七种方块，随机生成一种，并随机选择小方块的颜色
* 初始化每个小方块在界面上的位置，用于后续渲染
*/
Block::Block()
{
	//切割图片，存入数组中
	if (imgs[0] == nullptr) {
		IMAGE imgTmp;
		loadimage(&imgTmp,"res/cubeColor.png");
		SetWorkingImage(&imgTmp);
		for (int i = 0; i < 7; ++i) {
			imgs[i] = new IMAGE;//申请内存
			getimage(imgs[i], i*imgSize,0,imgSize,imgSize);//开始切割，输入位置和切割大小
		}
		SetWorkingImage();//恢复工作区
	}

	//七种方块
	// 0 1
	// 2 3
	// 4 5
	// 6 7
	//用4行2列的二维数组表示，选择其中的四个数代表一种方块
	int blocks[7][4] = {
		1,3,5,7,//I型
		2,4,5,7,//Z型1
		3,5,4,6,//Z型2
		3,5,4,7,//T
		2,3,5,7,//L
		3,5,7,6,//J
		2,3,4,5	//田
	};

	//随机生成一种
	blockType = rand() % 7;//0~6

	//需要确定方块在图片中的第几行第几列
	//初始化每个小方块位置smallBlocks:默认位置是左上角，若要设置在中间，则需要知道游戏区域的行和列
	for (int i = 0; i < 4; ++i) {
		int val = blocks[blockType][i];
		smallBlocks[i].row = val / 2;
		smallBlocks[i].col = val % 2;
	}

	//数据绑定图片
	img = imgs[blockType];

}

//赋值构造
Block& Block::operator=(const Block& other)
{
	// TODO: 在此处插入 return 语句
	if (this == &other) return *this;

	this->blockType = other.blockType;
	for (int i = 0; i < 4;++i) {
		this->smallBlocks[i] = other.smallBlocks[i];
	}
}

//自动下降:四个小方块坐标下降一格
void Block::drop()
{
	for (int i = 0; i < 4; ++i) {
		smallBlocks[i].row++;
	}
}

void Block::move(int offset)
{
	for (auto& b : smallBlocks) {
		b.col+=offset;
	}
}

void Block::rotate()
{
	//方块旋转

	//确定旋转中心
	Point p = smallBlocks[1];

	for (int i = 0; i < 4; ++i) {
		Point tmp = smallBlocks[i];
		smallBlocks[i].col = p.col - tmp.row + p.row;
		smallBlocks[i].row = p.row + tmp.col - p.col;
	}
}

//在每个方块位置渲染生成方块
void Block::draw(int edge_left, int edge_top, int size)
{
	for (int i = 0; i < 4; ++i) {
		//确定每个小方块的坐标
		int x = edge_left + smallBlocks[i].col * imgSize;
		int y = edge_top + smallBlocks[i].row * imgSize;
		putimage(x, y, img);
	}
}

IMAGE** Block::getImgs()
{
	return imgs;
}

bool Block::blocksInGrids(const vector<vector<int>>& grids)
{
	int rows = grids.size();
	int cols = grids[0].size();
	for (auto& b : smallBlocks) {
		if (b.row < 0 || b.row >= rows || b.col < 0 || b.col >= cols || grids[b.row][b.col] != -1) return false;
	}
	return true;
}

void Block::solidify(vector<vector<int>>& grids)
{
	for (auto& b : smallBlocks) {
		//冻结方块，改写网格数据
		grids[b.row][b.col] = blockType;

	}
}

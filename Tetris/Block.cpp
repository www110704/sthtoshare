#include "Block.h"
#include<stdlib.h>

IMAGE* Block::imgs[7] = { nullptr, };
int Block::imgSize = 40;

/*��ʼ�������ͼ��
* ������ɫ�ز�ͼ�󣬷ָ����ͼ��ָ��������
* ���ַ��飬�������һ�֣������ѡ��С�������ɫ
* ��ʼ��ÿ��С�����ڽ����ϵ�λ�ã����ں�����Ⱦ
*/
Block::Block()
{
	//�и�ͼƬ������������
	if (imgs[0] == nullptr) {
		IMAGE imgTmp;
		loadimage(&imgTmp,"res/cubeColor.png");
		SetWorkingImage(&imgTmp);
		for (int i = 0; i < 7; ++i) {
			imgs[i] = new IMAGE;//�����ڴ�
			getimage(imgs[i], i*imgSize,0,imgSize,imgSize);//��ʼ�и����λ�ú��и��С
		}
		SetWorkingImage();//�ָ�������
	}

	//���ַ���
	// 0 1
	// 2 3
	// 4 5
	// 6 7
	//��4��2�еĶ�ά�����ʾ��ѡ�����е��ĸ�������һ�ַ���
	int blocks[7][4] = {
		1,3,5,7,//I��
		2,4,5,7,//Z��1
		3,5,4,6,//Z��2
		3,5,4,7,//T
		2,3,5,7,//L
		3,5,7,6,//J
		2,3,4,5	//��
	};

	//�������һ��
	blockType = rand() % 7;//0~6

	//��Ҫȷ��������ͼƬ�еĵڼ��еڼ���
	//��ʼ��ÿ��С����λ��smallBlocks:Ĭ��λ�������Ͻǣ���Ҫ�������м䣬����Ҫ֪����Ϸ������к���
	for (int i = 0; i < 4; ++i) {
		int val = blocks[blockType][i];
		smallBlocks[i].row = val / 2;
		smallBlocks[i].col = val % 2;
	}

	//���ݰ�ͼƬ
	img = imgs[blockType];

}

//��ֵ����
Block& Block::operator=(const Block& other)
{
	// TODO: �ڴ˴����� return ���
	if (this == &other) return *this;

	this->blockType = other.blockType;
	for (int i = 0; i < 4;++i) {
		this->smallBlocks[i] = other.smallBlocks[i];
	}
}

//�Զ��½�:�ĸ�С���������½�һ��
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
	//������ת

	//ȷ����ת����
	Point p = smallBlocks[1];

	for (int i = 0; i < 4; ++i) {
		Point tmp = smallBlocks[i];
		smallBlocks[i].col = p.col - tmp.row + p.row;
		smallBlocks[i].row = p.row + tmp.col - p.col;
	}
}

//��ÿ������λ����Ⱦ���ɷ���
void Block::draw(int edge_left, int edge_top, int size)
{
	for (int i = 0; i < 4; ++i) {
		//ȷ��ÿ��С���������
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
		//���᷽�飬��д��������
		grids[b.row][b.col] = blockType;

	}
}

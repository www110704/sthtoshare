/*开发日志
* 1.创建项目
* 2.先导入素材（美工）
* 3.c++开发
* 4.设计C++项目的模块（类）
	方块类Block
	游戏类Tetris
* 5.设计各个模块主要接口
* 6.启动游戏
* 7.
* 


*/


#include<iostream>
#include"Tetris.h"

int main() {
	Tetris game(20,10,262,157,40);
	game.start();
	return 0;
}
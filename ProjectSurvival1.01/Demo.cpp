#include<iostream>
#include<graphics.h>

char board_date[3][3] = {
	'.' ,'.' ,'.' ,
	'.' ,'.' ,'.' ,
	'.' ,'.' ,'.' };//棋盘初始状态

char current_chess = 'o';//当前落子

int chess_count = 0;

//检测胜利
bool CheckWin(char c) {
	if (board_date[0][0] == c && board_date[0][1] == c && board_date[0][2] == c) return true;
	if (board_date[1][0] == c && board_date[1][1] == c && board_date[1][2] == c) return true;
	if (board_date[2][0] == c && board_date[2][1] == c && board_date[2][2] == c) return true;
	if (board_date[0][0] == c && board_date[1][0] == c && board_date[2][0] == c) return true;
	if (board_date[0][1] == c && board_date[1][1] == c && board_date[2][1] == c) return true;
	if (board_date[0][2] == c && board_date[1][2] == c && board_date[2][2] == c) return true;
	if (board_date[0][0] == c && board_date[1][1] == c && board_date[2][2] == c) return true;
	if (board_date[0][2] == c && board_date[1][1] == c && board_date[2][0] == c) return true;
	return false;
}

//检测平局
bool CheckDraw() {
	return chess_count == 9;
}

void DrawGrids() {
	line(0, 200, 600, 200);
	line(0, 400, 600, 400);
	line(200, 0, 200, 600);
	line(400, 0, 400, 600);
}

void DrawChess() {
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			if (board_date[i][j] == 'o') {
				circle(j * 200 + 100, i * 200 + 100, 100);
			}
			else if (board_date[i][j] == 'x') {
				line(j * 200, i * 200, (j + 1) * 200, (i + 1) * 200);
				line((j + 1) * 200, i * 200, j * 200, (i + 1) * 200);
			}
		}
	}
}

//编码类型：char,wchar_t,TCHAR,LPCTSTR,string,wstring,unicode,GBK,UTF_8
//https://www.cnblogs.com/imlucky/archive/2013/05/10/3070581.html
void DrawTipText() {
	static TCHAR str[64];
	_stprintf_s(str, _T("当前的落子类型是：%c"), current_chess);
	
	settextcolor(RGB(255, 175, 45));
	outtextxy(0, 0, str);
}



//井字棋游戏demo
int main() {

	//printf("%ws", GetEasyXVer());

	initgraph(600, 600);

	int x, y;

	ExMessage msg;

	bool running = true;

	BeginBatchDraw();

	while (running) {
		DWORD start = GetTickCount();

		//读取输入()
		while (peekmessage(&msg)) {//读取消息队列

			//处理数据()
			if (msg.message == WM_LBUTTONDOWN&& msg.x>=0&&msg.x<=600&&msg.y>=0&&msg.y<=600) {//鼠标左键按下事件
				x = msg.x;
				y = msg.y;

				int index_x = x / 200;
				int index_y = y / 200;

				//落子
				if (board_date[index_y][index_x] == '.') {
					++chess_count;
					board_date[index_y][index_x] = current_chess;

					//切换落子
					if (current_chess == 'o') current_chess = 'x';
					else current_chess = 'o';
				}
				
			}

		}

		cleardevice();//清空画布

		DrawGrids();
		DrawChess();
		DrawTipText();

		FlushBatchDraw();//画面双缓冲机制，防止画面闪烁


		if (CheckWin('x')) {
			//提示弹窗(父弹窗句柄，提示内容，弹窗标题，弹窗样式);
			MessageBox(GetHWnd(), _T("x玩家获胜！"), _T("游戏结束！"), MB_OK);
			running = false;
		}
		else if (CheckWin('o')) {
			MessageBox(GetHWnd(), _T("o玩家获胜！"), _T("游戏结束！"), MB_OK);
			running = false;
		}
		else if (CheckDraw()) {
			MessageBox(GetHWnd(), _T("平局！"), _T("游戏结束！"), MB_OK);
			running = false;
		}
		
		DWORD end = GetTickCount();
		DWORD delay_time = end - start;
		//限制刷新率
		if (delay_time < 1000 / 60) {
			Sleep(1000 / 60 - delay_time);
		}
	}


	EndBatchDraw();

	return 0;
}
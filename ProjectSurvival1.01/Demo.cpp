#include<iostream>
#include<graphics.h>

char board_date[3][3] = {
	'.' ,'.' ,'.' ,
	'.' ,'.' ,'.' ,
	'.' ,'.' ,'.' };//���̳�ʼ״̬

char current_chess = 'o';//��ǰ����

int chess_count = 0;

//���ʤ��
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

//���ƽ��
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

//�������ͣ�char,wchar_t,TCHAR,LPCTSTR,string,wstring,unicode,GBK,UTF_8
//https://www.cnblogs.com/imlucky/archive/2013/05/10/3070581.html
void DrawTipText() {
	static TCHAR str[64];
	_stprintf_s(str, _T("��ǰ�����������ǣ�%c"), current_chess);
	
	settextcolor(RGB(255, 175, 45));
	outtextxy(0, 0, str);
}



//��������Ϸdemo
int main() {

	//printf("%ws", GetEasyXVer());

	initgraph(600, 600);

	int x, y;

	ExMessage msg;

	bool running = true;

	BeginBatchDraw();

	while (running) {
		DWORD start = GetTickCount();

		//��ȡ����()
		while (peekmessage(&msg)) {//��ȡ��Ϣ����

			//��������()
			if (msg.message == WM_LBUTTONDOWN&& msg.x>=0&&msg.x<=600&&msg.y>=0&&msg.y<=600) {//�����������¼�
				x = msg.x;
				y = msg.y;

				int index_x = x / 200;
				int index_y = y / 200;

				//����
				if (board_date[index_y][index_x] == '.') {
					++chess_count;
					board_date[index_y][index_x] = current_chess;

					//�л�����
					if (current_chess == 'o') current_chess = 'x';
					else current_chess = 'o';
				}
				
			}

		}

		cleardevice();//��ջ���

		DrawGrids();
		DrawChess();
		DrawTipText();

		FlushBatchDraw();//����˫������ƣ���ֹ������˸


		if (CheckWin('x')) {
			//��ʾ����(�������������ʾ���ݣ��������⣬������ʽ);
			MessageBox(GetHWnd(), _T("x��һ�ʤ��"), _T("��Ϸ������"), MB_OK);
			running = false;
		}
		else if (CheckWin('o')) {
			MessageBox(GetHWnd(), _T("o��һ�ʤ��"), _T("��Ϸ������"), MB_OK);
			running = false;
		}
		else if (CheckDraw()) {
			MessageBox(GetHWnd(), _T("ƽ�֣�"), _T("��Ϸ������"), MB_OK);
			running = false;
		}
		
		DWORD end = GetTickCount();
		DWORD delay_time = end - start;
		//����ˢ����
		if (delay_time < 1000 / 60) {
			Sleep(1000 / 60 - delay_time);
		}
	}


	EndBatchDraw();

	return 0;
}
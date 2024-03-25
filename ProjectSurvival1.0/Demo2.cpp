//#include<iostream>
//#include<graphics.h>
//#include<string>
//
//int idx_cur_animation = 0;//当前的动画帧索引
//const int PLAYER_ANIM_NUM = 6;
//int PLAYER_SPEED = 2;
//
//IMAGE img_player_left[PLAYER_ANIM_NUM];
//IMAGE img_player_right[PLAYER_ANIM_NUM];
//
//int windows_width = 1280;
//int windows_height = 720;
//POINT player_pos= { windows_width/2 ,windows_height/2 };
//
//bool is_move_left = false;
//bool is_move_right = false;
//bool is_move_up = false;
//bool is_move_down = false;
//
//void loadAnimation() {
//	for (int i = 0; i < PLAYER_ANIM_NUM; ++i) {
//		std::wstring path_l = L"res/img/player_left_" + std::to_wstring(i) + L".png";
//		std::wstring path_r = L"res/img/player_right_" + std::to_wstring(i) + L".png";
//		loadimage(&img_player_left[i], path_l.c_str());
//		loadimage(&img_player_right[i], path_r.c_str());
//	}
//}
//
//
//
//inline void putimage_alpha(int x,int y,IMAGE* img) {
//	int w = img->getwidth();
//	int h = img->getheight();
//	//渲染含有透明像素的位图
//	AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER,0,255,AC_SRC_ALPHA });
//}
//
//
//int main() {
//
//	//printf("%ws", GetEasyXVer());
//	
//	initgraph(windows_width, windows_height);
//
//	bool running = true;
//
//	IMAGE img_background;
//	loadimage(&img_background, _T("res/img/background.png"));
//
//	loadAnimation();
//
//	BeginBatchDraw();
//
//	while (running) {
//		DWORD start_time = GetTickCount();
//
//
//		ExMessage msg;
//		//读取输入()
//		while (peekmessage(&msg)) {//读取消息列表
//			//处理数据()
//			if (msg.message == WM_KEYDOWN) {//键盘按下,不直接计算位移,而是开启位移判断
//				switch (msg.vkcode) {
//				case VK_UP:case 0x57://W
//					//player_pos.y -= PLAYER_SPEED;
//					is_move_up = true;
//					break;
//				case VK_DOWN:case 0x53://S
//					//player_pos.y += PLAYER_SPEED;
//					is_move_down = true;
//					break;
//				case VK_LEFT:case 0x41://A
//					//player_pos.x -= PLAYER_SPEED;
//					is_move_left = true;
//					break;
//				case VK_RIGHT:case 0x44://D
//					//player_pos.x += PLAYER_SPEED;
//					is_move_right = true;
//					break;
//				}
//			}
//			else if (msg.message == WM_KEYUP) {//键盘释放
//				switch (msg.vkcode) {
//				case VK_UP:case 0x57:
//					is_move_up = false;
//					break;
//				case VK_DOWN:case 0x53:
//					is_move_down = false;
//					break;
//				case VK_LEFT:case 0x41:
//					is_move_left = false;
//					break;
//				case VK_RIGHT:case 0x44:
//					is_move_right = false;
//					break;
//				}
//			}
//
//		}
//
//		//如何解决斜着移动速度是根号2倍的问题
//		if (is_move_up) player_pos.y -= PLAYER_SPEED;
//		if (is_move_down) player_pos.y += PLAYER_SPEED;
//		if (is_move_left) player_pos.x -= PLAYER_SPEED;
//		if (is_move_right) player_pos.x += PLAYER_SPEED;
//
//
//		static int anim_counter = 0;//当前动画帧计数
//
//		if (++anim_counter % 5== 0) ++idx_cur_animation;//每5个游戏帧切换一个动画
//
//		idx_cur_animation = idx_cur_animation%PLAYER_ANIM_NUM;//循环播放动画
//			
//		
//
//		//更新画面()
//		cleardevice();//清空画布
//
//		putimage(0, 0, &img_background);//绘制背景
//
//		//solidcircle(x, y, 100);
//		putimage_alpha(player_pos.x, player_pos.y, &img_player_left[idx_cur_animation]);//绘制人物
//
//		FlushBatchDraw();//画面双缓冲机制，防止画面闪烁
//
//
//
//		//设置刷新延迟，减少CPU消耗
//		DWORD end_time = GetTickCount();
//		DWORD delt_time = end_time - start_time;
//		if (delt_time < 1000 / 144) {
//			Sleep(1000 / 144 - delt_time);
//		}
//
//	}
//
//
//	EndBatchDraw();
//
//	return 0;
//}
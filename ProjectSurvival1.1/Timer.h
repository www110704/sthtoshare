#pragma once
#include<iostream>
#include<functional>


class Timer {
public:
	Timer() = default;
	~Timer() = default;

	void on_update(int delay)
	{
		if (paused) return;
		pass_time += delay;

		if (pass_time >= wait_time) {
			//���Դ�����ʱ���ص�
			if (callback && (!one_shot || (one_shot && !shotted)))//ѭ������ ���� һ�ζ�û������
				callback();
			shotted = true;
			pass_time = 0;
		}
	}

	void restart() {//������ʱ��
		pass_time = 0;
		shotted = false;
	}

	void set_wait_time(int val)
	{
		wait_time = val;
	}

	void set_one_shot(bool flag) {
		one_shot = flag;
	}

	void set_callback(std::function<void()> callback) {
		this->callback = callback;
	}

	void pause()
	{
		paused = true;
	}

	void resum()
	{
		paused = false;
	}

private:
	int pass_time = 0;			//�ѵ�ʱ��	
	int wait_time = 0;			//�ȴ�ʱ��
	bool paused = false;		//�Ƿ���ͣ
	bool shotted = false;		//�Ƿ񴥷�
	bool one_shot = false;		//���δ���

	std::function<void()> callback;
};
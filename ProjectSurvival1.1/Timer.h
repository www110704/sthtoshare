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
			//尝试触发定时器回调
			if (callback && (!one_shot || (one_shot && !shotted)))//循环触发 或者 一次都没触发过
				callback();
			shotted = true;
			pass_time = 0;
		}
	}

	void restart() {//重启定时器
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
	int pass_time = 0;			//已等时间	
	int wait_time = 0;			//等待时间
	bool paused = false;		//是否暂停
	bool shotted = false;		//是否触发
	bool one_shot = false;		//单次触发

	std::function<void()> callback;
};
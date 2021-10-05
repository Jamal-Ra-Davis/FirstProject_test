#pragma once
#include <Windows.h>
#include <stdint.h>

class Serial_Object {
public:
	Serial_Object() {}
	void print(const char* str) { printf("%s", str); }
	void print(float val) { printf("%f", val); }
	void print(int val) { printf("%d", val); }
	void println(const char* str) { printf("%s\n", str); }
	void println(float val) { printf("%f\n", val); }
	void println(int val) { printf("%d\n", val); }
	void println() { printf("\n"); }
};
Serial_Object Serial1;
extern Serial_Object Serial1;

Serial_Object SerialUSB;
extern Serial_Object SerialUSB;

inline int64_t systime_to_timestamp(SYSTEMTIME* curr_time)
{
	int64_t ts = (int64_t)curr_time->wMilliseconds +
		(1000 * (int64_t)curr_time->wSecond) +
		(60 * 1000 * (int64_t)curr_time->wMinute) +
		(60 * 60 * 1000 * (int64_t)curr_time->wHour);
	return ts;
}
inline void delay(int ms)
{
	SYSTEMTIME ts;
	GetSystemTime(&ts);

	while (1)
	{
		SYSTEMTIME ts_new;
		GetSystemTime(&ts_new);
		int64_t delta = systime_to_timestamp(&ts_new) - systime_to_timestamp(&ts);

		if (delta >= ms)
			break;
	}
}
void delayMicroseconds(int us)
{
	if (us < 2000) {
		delay(1);
	}
	else {
		delay(us / 1000);
	}
}

class rtc_obj {
public:
	rtc_obj() {}
	int getSeconds();
	int getMinutes();
	int getHours() { return 8; }
};
int rtc_obj::getSeconds()
{
	int seconds = ((int)glfwGetTime()) % 60;
	return seconds;
}
int rtc_obj::getMinutes()
{
	int seconds = ((int)glfwGetTime()) % 3600;
	int minutes = seconds / 60;
	return minutes;
}
rtc_obj rtc;
extern rtc_obj rtc;
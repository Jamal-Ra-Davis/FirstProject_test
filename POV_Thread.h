#pragma once

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include "Arduino.h"
#include "Shell.h"
#include "RingBuf.h"		//Including here to avoid Events.h from including external RingBuf lib, but want to figure out a more graceful way
#include <pov_display/Color.h>
#include <pov_display/FrameBuffer.h>
#include <pov_display/Events.h>
#include <pov_display/Text.h>
#include <pov_display/test_animations.h>
#include <pov_display/Space_Game.h>
#include <pov_display/Main.h>


#define TICK_DELAY 5
#define PRINT_DELTA_TIME false

struct ButtonStatus {
	typedef enum {BTN_NONE, BTN_PRESS, BTN_RELEASE} button_status_t;
	uint32_t button_state_prev;
	button_status_t button_events[NUM_KEYS];
};
struct ThreadData {
	bool thread_running;
	SYSTEMTIME prev_thread_time;
};

void delay_ms(int ms, bool *thread_running, SYSTEMTIME *ts)
{
	while (1)
	{
		if (*thread_running == false)
			break;

		SYSTEMTIME ts_new;
		GetSystemTime(&ts_new);
		int64_t delta = systime_to_timestamp(&ts_new) - systime_to_timestamp(ts);

		if (delta >= ms)
			break;
	}
}
void clock_test(doubleBuffer* frame_buffer);
void test_exec(doubleBuffer* frame_buffer);
void main_exec(doubleBuffer* frame_buffer);

void processEvents(struct ButtonStatus *button_status)
{
	for (int i = 0; i < NUM_KEYS; i++)
	{
		if (button_status->button_events[i] == ButtonStatus::BTN_PRESS)
		{
			eventBuffer.push(Event(Event::ON_PRESS, i));
			printf("Button Pressed: %d\n", i);
		}
		else if (button_status->button_events[i] == ButtonStatus::BTN_RELEASE)
		{
			eventBuffer.push(Event(Event::ON_RELEASE, i));
			printf("Button Released: %d\n", i);
		}
		button_status->button_events[i] = ButtonStatus::BTN_NONE;
	}
}
void thread_setup(struct ThreadData* thread_data, doubleBuffer* frame_buffer, struct ButtonStatus *button_status)
{
	GetSystemTime(&thread_data->prev_thread_time);
	main_setup(frame_buffer);
}

void thread_loop(struct ThreadData* thread_data, doubleBuffer* frame_buffer, struct ButtonStatus* button_status)
{
	while (thread_data->thread_running)
	{
		SYSTEMTIME ts;
		GetSystemTime(&ts);
		if (PRINT_DELTA_TIME)
		{
			int64_t delta = systime_to_timestamp(&ts) - systime_to_timestamp(&thread_data->prev_thread_time);
			printf("Delta time: %lld ms\n", delta);
		}
		thread_data->prev_thread_time = ts;
		
		processEvents(button_status);
		frame_buffer->clear();

		main_exec(frame_buffer);//exec function responsible for managing event buffer

		frame_buffer->update();
		delay_ms(TICK_DELAY, &thread_data->thread_running, &ts);
	}
}

void thread_main(struct ThreadData *thread_data, doubleBuffer* frame_buffer, struct ButtonStatus* button_status)
{
	thread_setup(thread_data, frame_buffer, button_status);//Equivalent of arduino setup()
	thread_loop(thread_data, frame_buffer, button_status);//Equivalent of superLoop()
}

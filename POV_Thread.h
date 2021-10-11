#pragma once

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include "Arduino.h"
#include "Shell.h"
#include "RingBuf.h"		//Including here to avoid Events.h from including external RingBuf lib, but want to figure out a more graceful way
#include <pov_display/Color.h>
//#include "FrameBuffer.h"
#include <pov_display/FrameBuffer.h>
//#include "Events.h"
#include <pov_display/Events.h>

//#include "Text.h"
#include <pov_display/Text.h>
//#include "test_animations.h"
#include <pov_display/test_animations.h>
//#include "Space_Game.h"
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

/*
pov_state_t exec_state;
bool pov_state_change = true;
int change_state(pov_state_t state)
{
	if (exec_state != state)
	{
		exec_state = state;
		pov_state_change = true;
	}
	return 0;
}
*/
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
	/*
	frame_buffer->reset();
	exec_state = SPACE_GAME;
	Color c = Color::getColorHSV(200, 255, 255);
	printf("Test color = (%d, %d, %d)\n", c.r, c.g, c.b);
	*/
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




/*
void clock_test(doubleBuffer* frame_buffer)
{
	//Clock
	int sec = rtc.getSeconds();
	int min = rtc.getMinutes();
	int hour = rtc.getHours() % 12;
	static int h_off = 0;
	static int h_up = 1;
	static int h_inc = 0;

	static const uint8_t delay_cnt = 5;
	static uint8_t cnt = 0;

	//Draw back plane
	for (int i = 0; i < LENGTH; i++)
	{
		frame_buffer->setColors(i, WIDTH - 1, h_off, 255, 255, 255);
		if (i % (LENGTH / 12) == 0)
		{
			frame_buffer->setColors(i, WIDTH - 2, h_off, 255, 255, 255);
			frame_buffer->setColors(i, WIDTH - 3, h_off, 255, 255, 255);
		}
	}

	//Draw Hands
	for (int j = 0; j < WIDTH; j++)
	{
		//Draw sec hand
		int sec_idx = (sec * LENGTH) / 60;
		if (j <= WIDTH - 3)
		{
			frame_buffer->setColors(sec_idx, j, 1 + h_off, 0, 0, 255);
		}

		//Draw min hand
		int min_idx = (min * LENGTH) / 60;
		if (j <= WIDTH - 3)
		{
			frame_buffer->setColors(min_idx, j, 2 + h_off, 0, 255, 0);
		}

		//Draw hour hand
		int hour_idx = (hour * LENGTH) / 12;
		if (j <= WIDTH - 5)
		{
			frame_buffer->setColors(hour_idx, j, 3 + h_off, 255, 0, 0);
		}
	}

	//Update logic
	if (cnt++ % delay_cnt == 0)
	{
		h_inc++;
		if ((h_inc % 77) == 0)
		{
			if (h_up)
			{
				h_off++;
				if (h_off > 2)
				{
					h_off = 2;
					h_up = 0;
				}
			}
			else
			{
				h_off--;
				if (h_off < 0)
				{
					h_off = 0;
					h_up = 1;
				}
			}
		}
		//SERIAL_PRINTF(SerialUSB, "%02d:%02d:%02d\n", hour, min, sec);
	}
}
*/
/*
void test_exec(doubleBuffer* frame_buffer)
{
	static const uint8_t delay_cnt = 10;
	static uint8_t cnt = 0;
	static const char* test_message = "HI NOLAN";

	static int message_len = strlen(test_message);
	static int start_offset = -1 * message_len * 8;
	static int offset = start_offset;

	static int start_offset2 = LENGTH;
	static int offset2 = start_offset;

	//Draw
	writeString(test_message, offset, 1, 50, 0, 255, frame_buffer);
	writeString(test_message, offset2, 4, 0, 255, 100, frame_buffer);

	//Update logic
	if (cnt++ % delay_cnt == 0)
	{
		offset++;
		if (offset >= LENGTH)
		{
			offset = start_offset;
		}

		//SERIAL_PRINTF(SerialUSB, "Offset2 = %d\n", offset2);
		offset2--;
		//SERIAL_PRINTF(SerialUSB, "Offset2(post) = %d\n", offset2);
		if (offset2 <= -1 * message_len * 8)
		{
			//SERIAL_PRINTF(SerialUSB, "Offset2 = %d, -1*message_len*8 = %d\n", offset2, -1*message_len*8);
			offset2 = start_offset2;
		}
	}
}
*/
/*
void main_exec_(doubleBuffer* frame_buffer)
{
	const static int TICK_PERIOD = 19;
	static int idx = 0;
	//Update framebuffer
	for (int i = 0; i < 96; i++)
	{
		if (i % 6 == 0)
		{
			for (int k = 0; k < 6; k++)
			{
				frame_buffer->setColors(i, 0, k, (255 / 6) * k, 0, (255 / 6) * (5 - k));
			}
		}
		frame_buffer->setColors(i, (i + idx / TICK_PERIOD) % 8, 3, 0, 255, 0);

	}
	frame_buffer->setColors((idx / TICK_PERIOD) % 96, 7, 3, 255, 0, 0);
	idx++;
}
*/
/*
SpaceGame space_game;
void main_exec(doubleBuffer* frame_buffer)
{
	switch (exec_state)
	{
		case POV_SCRATCH_LOOP:
			//scratch_loop();
			break;
		case POV_TEST:
			//test_exec(frame_buffer);
			//textAnimation(frame_buffer);
			//pinWheelAnimation_0(frame_buffer);
			vortexAnimation(frame_buffer);
			//pulseAnimation(frame_buffer);
			break;
		case DS4_TEST:
			//ds4_test();
			//ds4_analog_test();
			break;
		case SPACE_GAME:
			space_game.update();
			space_game.draw(frame_buffer);
			break;
		case CLOCK_DISPLAY:
			clock_test(frame_buffer);
			break;
		default:
			break;
	}
}
*/

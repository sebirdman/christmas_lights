/* 
 * Copyright (c) 2017 pcbreflux. All Rights Reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "sdkconfig.h"

#include "WS2812.h"
#include "lights_task.h"

#define TAG "WS2812_TASK"

#define WS2812_GIPO GPIO_NUM_17
#define WS2812_PIXEL_COUNT 150

static WS2812 *oWS2812;

const uint8_t sine_pattern[16] = {127, 176, 217, 245,
								  254, 245, 217, 176,
								  127, 79, 38, 10,
								  0, 10, 38, 79};

uint8_t random(uint8_t min, uint8_t max)
{
	if (min > max)
	{
		uint8_t swap;
		swap = min;
		min = max;
		max = swap;
	}
	return (uint8_t)(min + esp_random() % (max + 1 - min));
}

// Taken from: https://github.com/uncle-yong/sk1632-ws2812-christmas
void runningLightsWave(uint32_t pixelCount, uint32_t loops, uint32_t startColor, bool shift, uint16_t delayms)
{

	uint8_t shift_ctr = 0;

	uint8_t s;

	uint8_t r = ((startColor >> 16) & 0xFF);
	uint8_t g = ((startColor >> 8) & 0xFF);
	uint8_t b = (startColor & 0xFF);

	for (auto i = 0; i < loops; i++)
	{
		if (shift)
			shift_ctr++;

		for (auto j = 0; j < pixelCount; j++)
		{
			//s = (uint16_t)((sin(2.00*3.142*i/16.00) * 127.00) + 128.00);       // <- spread i to the number of pixels available!
			if (shift)
				s = sine_pattern[(j + shift_ctr) & 0x0f];
			else
				s = sine_pattern[j & 0x0f];

			oWS2812->setPixel(j, (r * s) / 256, (g * s) / 256, (b * s) / 256);
		}

		oWS2812->show();
		vTaskDelay(delayms / portTICK_PERIOD_MS);
	}
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int rgb2hue(uint32_t pixel)
{
	const uint8_t r = pixel & 0xFF;
	const uint8_t b = (pixel & 0xff00) >> 8;
	const uint8_t g = (pixel & 0xff0000) >> 16;

	const uint8_t red = r / 255;
	const uint8_t green = g / 255;
	const uint8_t blue = b / 255;

	uint8_t max = MAX(MAX(red, green), blue);
	uint8_t min = MIN(MIN(red, green), blue);
	uint8_t c = max - min;
	uint8_t hue = 0;

	if (c == 0)
	{
		return 0;
	}
	else
	{
		if (max == red)
		{
			uint8_t segment = (green - blue) / c;
			uint8_t shift = 0 / 60; // R° / (360° / hex sides)
			if (segment < 0)
			{					  // hue > 180, full rotation
				shift = 360 / 60; // R° / (360° / hex sides)
			}
			hue = segment + shift;
		}
		else if (max == blue)
		{
			uint8_t segment = (red - green) / c;
			uint8_t shift = 240 / 60; // B° / (360° / hex sides)
			hue = segment + shift;
		}
		else if (max == green)
		{
			uint8_t segment = (blue - red) / c;
			uint8_t shift = 120 / 60; // G° / (360° / hex sides)
			hue = segment + shift;
		}
	}
	return hue * 60; // hue is in [0,6], scale it up
}

// Taken from: https://github.com/uncle-yong/sk1632-ws2812-christmas
void alt2Colors(uint16_t pixelCount, uint32_t loops, uint32_t color1, uint32_t color2, uint16_t delayms)
{

	for (auto i = 0; i < loops; i++)
	{
		for (uint32_t pos = 0; pos < 31; pos++)
		{
			// Even numbers:
			for (uint16_t i = 0; i < pixelCount; i += 2)
			{
				oWS2812->setHSBPixel(i, rgb2hue(color1), 255, pos * 4);
			}
			// Odd numbers:
			for (uint16_t i = 1; i < pixelCount; i += 2)
			{
				oWS2812->setHSBPixel(i, rgb2hue(color2), 255, pos * 4);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}

		// Flip this:
		// Even numbers:
		for (uint16_t i = 0; i < pixelCount; i += 2)
		{
			oWS2812->setHSBPixel(i, rgb2hue(color2), 255, 100);
		}
		// Odd numbers:
		for (uint16_t i = 1; i < pixelCount; i += 2)
		{
			oWS2812->setHSBPixel(i, rgb2hue(color1), 255, 100);
		}

		oWS2812->show();
		vTaskDelay(delayms / portTICK_PERIOD_MS);
	}
}

void ramdomBlink(uint16_t pixelCount, uint32_t loops, uint16_t delayms)
{

	ESP_LOGI(TAG, "ramdomBlink %d", loops);
	uint8_t bpos, bdir;

	bpos = 0;
	bdir = 0;
	for (uint32_t pos = 0; pos < loops; pos++)
	{
		for (uint16_t i = 0; i < pixelCount; i++)
		{

			oWS2812->setPixel(i, random(0, 255), random(0, 255), random(0, 255));
		}
		if (bdir == 0)
		{
			bpos++;
		}
		else
		{
			bpos--;
		}
		if (bpos >= 31)
		{
			bpos = 30;
			bdir = 1;
		}
		if (bpos == 0)
		{
			bpos = 1;
			bdir = 0;
		}
		oWS2812->show();
		vTaskDelay(delayms / portTICK_PERIOD_MS);
	}
}

void setColor(uint16_t pixelCount, pixel_t pixel)
{

	for (uint16_t i = 0; i < pixelCount; i++)
	{
		oWS2812->setPixel(i, pixel.red, pixel.green, pixel.blue);
	}
	oWS2812->show();
}

void fadingRainbow(uint16_t pixelCount, uint32_t loops, uint16_t delayms)
{
	double dHue;
	uint16_t hue;

	//ESP_LOGI(TAG, "fadingRainbow %d",pixelCount);
	for (auto i = 0; i < loops; i++)
	{
		for (uint32_t pos = 0; pos < 31; pos++)
		{
			for (uint16_t i = 0; i < pixelCount; i++)
			{
				dHue = (double)360 / pixelCount * (pos + i);
				hue = (uint16_t)dHue % 360;
				oWS2812->setHSBPixel(i, hue, 255, pos * 4);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
		for (uint32_t pos = 30; pos > 0; pos--)
		{
			for (uint16_t i = 0; i < pixelCount; i++)
			{
				dHue = (double)360 / pixelCount * (pos + i);
				hue = (uint16_t)dHue % 360;
				oWS2812->setHSBPixel(i, hue, 255, pos * 4);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
	}
}

void bluringRainbow(uint16_t pixelCount, uint32_t loops, uint16_t delayms)
{
	double dHue;
	uint16_t hue;

	//ESP_LOGI(TAG, "bluringRainbow %d",pixelCount);
	for (auto i = 0; i < loops; i++)
	{
		for (uint32_t pos = 0; pos < 31; pos++)
		{
			for (uint16_t i = 0; i < pixelCount; i++)
			{
				dHue = (double)360 / pixelCount * (pos + i);
				hue = (uint16_t)dHue % 360;
				oWS2812->setHSBPixel(i, hue, pos * 8, pos * 4);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
		for (uint32_t pos = 30; pos > 0; pos--)
		{
			for (uint16_t i = 0; i < pixelCount; i++)
			{
				dHue = (double)360 / pixelCount * (pos + i);
				hue = (uint16_t)dHue % 360;
				oWS2812->setHSBPixel(i, hue, pos * 8, pos * 4);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
	}
}

void bluringfullRainbow(uint16_t pixelCount, uint32_t loops, uint16_t delayms)
{
	double dHue;
	uint16_t hue;

	//ESP_LOGI(TAG, "bluringfullRainbow %d",pixelCount);
	for (auto i = 0; i < loops; i++)
	{
		for (uint32_t pos = 0; pos < 127; pos++)
		{
			for (uint16_t i = 0; i < pixelCount; i++)
			{
				dHue = (double)360 / 127 * (pos + i);
				hue = (uint16_t)dHue % 360;
				oWS2812->setHSBPixel(i, hue, pos * 2, pos);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
		for (uint32_t pos = 126; pos > 0; pos--)
		{
			for (uint16_t i = 0; i < pixelCount; i++)
			{
				dHue = (double)360 / 127 * (pos + i);
				hue = (uint16_t)dHue % 360;
				oWS2812->setHSBPixel(i, hue, pos * 2, pos);
			}
			oWS2812->show();
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
	}
}

void fadeInOutColor(uint16_t pixelCount, uint32_t loops, pixel_t pixel, uint16_t delayms)
{

	//ESP_LOGI(TAG, "fadeInOutColor %d",pixelCount);
	pixel_t cur_pixel;

	for (auto i = 0; i < loops; i++)
	{
		for (uint32_t pos = 0; pos < 31; pos++)
		{
			cur_pixel.red = (uint8_t)((uint32_t)pixel.red * pos / 32);
			cur_pixel.green = (uint8_t)((uint32_t)pixel.green * pos / 32);
			cur_pixel.blue = (uint8_t)((uint32_t)pixel.blue * pos / 32);

			setColor(pixelCount, cur_pixel);
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
		for (uint32_t pos = 30; pos > 0; pos--)
		{
			cur_pixel.red = (uint8_t)((uint32_t)pixel.red * pos / 32);
			cur_pixel.green = (uint8_t)((uint32_t)pixel.green * pos / 32);
			cur_pixel.blue = (uint8_t)((uint32_t)pixel.blue * pos / 32);

			setColor(pixelCount, cur_pixel);
			vTaskDelay(delayms / portTICK_PERIOD_MS);
		}
	}
}

void christmasWalk(uint16_t pixelCount, uint32_t loops, uint16_t delayms)
{

	//ESP_LOGI(TAG, "ramdomWalk %d",loops);
	for (uint32_t pos = 0; pos < loops; pos++)
	{
		for (uint16_t i = 0; i < pixelCount; i++)
		{
			if (i < (pos % (pixelCount)))
			{
				oWS2812->setPixel(i, 255, 0, 0);
			}
			else
			{
				oWS2812->setPixel(i, 0, 255, 0);
			}
		}
		oWS2812->show();
		vTaskDelay(delayms / portTICK_PERIOD_MS);
	}

	for (uint32_t pos = loops; pos > 0; pos--)
	{
		for (uint16_t i = 0; i < pixelCount; i++)
		{
			if (i < (pos % (pixelCount)))
			{
				oWS2812->setPixel(i, 0, 255, 0);
			}
			else
			{
				oWS2812->setPixel(i, 255, 0, 0);
			}
		}
		oWS2812->show();
		vTaskDelay(delayms / portTICK_PERIOD_MS);
	}
}

void ws2812_task(void *pvParameters)
{

	oWS2812 = new WS2812(WS2812_GIPO, WS2812_PIXEL_COUNT, RMT_CHANNEL_6);

	uint8_t pixelCount = WS2812_PIXEL_COUNT;
	uint8_t loops = 1;
	uint32_t color1 = 0xFF0000;
	uint32_t color2 = 0x0000FF;

	uint8_t fade_cycles = 31;
	uint8_t bottom_out = 1;

	uint32_t delayms = 65;

	uint32_t sat = 255;

	while (1)
	{

		for (auto i = 0; i < loops; i++)
		{
			for (uint32_t pos = bottom_out; pos < fade_cycles; pos++)
			{

				// Even numbers:
				for (uint16_t i = 0; i < pixelCount; i += 2)
				{
					oWS2812->setHSBPixel(i, rgb2hue(color1), sat, pos);
				}
				// Odd numbers:
				for (uint16_t i = 1; i < pixelCount; i += 2)
				{
					oWS2812->setHSBPixel(i, rgb2hue(color2), sat, pos);
				}
				oWS2812->show();
				vTaskDelay(delayms / portTICK_PERIOD_MS);
			}
			vTaskDelay(1000 / portTICK_PERIOD_MS);

			for (uint32_t pos = fade_cycles; pos > bottom_out; pos--)
			{

				// Even numbers:
				for (uint16_t i = 0; i < pixelCount; i += 2)
				{
					oWS2812->setHSBPixel(i, rgb2hue(color1), sat, pos);
				}
				// Odd numbers:
				for (uint16_t i = 1; i < pixelCount; i += 2)
				{
					oWS2812->setHSBPixel(i, rgb2hue(color2), sat, pos);
				}
				oWS2812->show();
				vTaskDelay(delayms / portTICK_PERIOD_MS);
			}
			uint32_t color2_new = color1;
			color1 = color2;
			color2 = color2_new;
		}
	}

	//ESP_LOGI(TAG, "All done!");

	vTaskDelete(NULL);
}
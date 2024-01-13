#include "main.h"
#include <cinttypes>
#include <iostream>
#include <fstream>
#include <string>

void initialize()
{
}

// set all ADI to output
pros::ADIDigitalOut clk(1, false);	 // clock
pros::ADIDigitalOut data1(2, false); // new note
pros::ADIDigitalOut data2(3, false); // note id
pros::ADIDigitalOut data3(4, false);
pros::ADIDigitalOut data4(5, false);
pros::ADIDigitalOut data5(6, false);
pros::ADIDigitalOut data6(7, false);
pros::ADIDigitalOut data7(8, false);

bool clockToggle = false;
void sendSignal(uint8_t value, bool newNote = true)
{
	// value must be below 64
	if (value > 63)
	{
		return;
	}

	// set new note
	data1.set_value(newNote ? 1 : 0);

	// data2 takes the least significant bit and so on
	data2.set_value(value & 1);
	data3.set_value((value >> 1) & 1);
	data4.set_value((value >> 2) & 1);
	data5.set_value((value >> 3) & 1);
	data6.set_value((value >> 4) & 1);
	data7.set_value((value >> 5) & 1);

	// toggle clock
	clk.set_value((int)clockToggle);
	clockToggle = !clockToggle;
}

const uint8_t correctTypeHeader[8] = {0x50, 0x49, 0x4D, 0x41, 0x4E, 0x56, 0x49, 0x44};

void opcontrol()
{
	int fps = 30;
	double frameDuration = (1000.0) / (double)fps;

	// create the canvas
	lv_obj_t *canvas = nullptr;
	canvas = lv_canvas_create(lv_scr_act(), NULL);

	std::ifstream vidfile;
	vidfile.open("/usd/vid.bindat");

	// read in the first 8 bytes
	uint8_t ftypeHeader[8];
	vidfile.read((char *)ftypeHeader, 8);

	// this needs to be PIMANVID
	for (int i = 0; i < 8; i++)
	{
		if (ftypeHeader[i] != correctTypeHeader[i])
		{
			return;
		}
	}

	// read in the frame count
	uint32_t fcount;
	vidfile.read((char *)&fcount, 4);

	// read in the frame duration
	uint32_t fsize;
	vidfile.read((char *)&fsize, 4);

	// this should be (480 * 240 )/8 + 1
	if (fsize != 14401)
	{
		return;
	}

	int startTime = pros::millis();
	int lastFrame = 0;
	int readFrames = -1;
	uint8_t *frame = new uint8_t[fsize];

	// decompressed buffer (original size is 480 * 240)
	lv_color_t *decomp = new lv_color_t[480 * 240];

	// since the brain can't handle <10 ms delay over ADI ports,
	// we will clock it higher and only send out signals when nessessary
	while (lastFrame < fcount)
	{
		int currentTime = pros::millis();
		double delta = currentTime - startTime;
		int frameID = (int)(delta / frameDuration);

		if (frameID != lastFrame)
		{

			// have we read this frame yet?
			if (frameID > readFrames)
			{
				// read in the frame
				vidfile.read((char *)frame, fsize);
				readFrames++;
			}
			lastFrame = frameID;

			// play audio
			uint8_t note = frame[fsize - 1];

			bool isStart = false;
			if (note > 64)
			{
				note -= 64;
				isStart = true;
			}

			sendSignal(note, isStart);

			// decompress the frame
			for (int i = 0; i < (480 * 240) / 8; i++)
			{
				uint8_t byte = frame[i];
				for (int j = 0; j < 8; j++)
				{
					uint8_t bit = (byte >> j) & 1;
					decomp[i * 8 + j] = bit ? LV_COLOR_BLACK : LV_COLOR_WHITE;
				}
			}

			lv_canvas_set_buffer(canvas, decomp, 480, 240, LV_IMG_CF_TRUE_COLOR);
		}


		// min delay is 2
		pros::delay(2);
	}

	printf("Done!\n");
}

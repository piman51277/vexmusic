#include "main.h"
#include <cinttypes>
#include <iostream>
#include <fstream>
#include <string>

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

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

	while (true)
	{

		printf("Starting video playback\n");

		// start the clock
		Clock c1;
		auto t1 = c1.now();

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

		auto t2 = c1.now();

		int headerReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
		printf("Header read in %d ms\n", headerReadTime);

		int startTime = pros::millis();
		int lastFrame = 0;
		int readFrames = -1;

		int fsize = 0;
		uint16_t frame[3000] = {};
		std::fill_n(frame, 3000, 0);

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

				bool firstBlack;
				uint16_t runCount;
				uint8_t note;
				if (frameID > readFrames)
				{
					auto t3 = c1.now();

					vidfile.read((char *)&runCount, 2);

					vidfile.read((char *)&firstBlack, 1);

					// read in the frame
					vidfile.read((char *)frame, runCount * 2);
					readFrames++;

					// read in the note
					vidfile.read((char *)&note, 1);

					auto t4 = c1.now();

					int frameReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
					printf("Frame %d read in %d ms\n", readFrames, frameReadTime);
				}

				lastFrame = frameID;

				// play audio

				bool isStart = false;
				if (note > 64)
				{
					note -= 64;
					isStart = true;
				}

				sendSignal(note, isStart);

				// decompress the frame
				bool color = firstBlack;
				int pix_ptr = 0;
				for (int rid = 0; rid < runCount; rid++)
				{
					for (int i = pix_ptr; i < pix_ptr + (int)frame[rid]; i++)
					{
						decomp[i] = color ? LV_COLOR_BLACK : LV_COLOR_WHITE;
					}
					color = !color;
					pix_ptr += frame[rid];
				}

				lv_canvas_set_buffer(canvas, decomp, 480, 240, LV_IMG_CF_TRUE_COLOR);
			}

			// min delay is 2
			pros::delay(2);
		}

		// clean up
		delete[] decomp;

		// close the file
		vidfile.close();

		// destroy the canvas
		lv_obj_del(canvas);

		pros::delay(1000);
	}

	printf("Done!\n");
}

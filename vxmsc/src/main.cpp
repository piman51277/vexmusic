#include "main.h"
#include <cinttypes>
#include <iostream>
#include <fstream>
#include <string>

void initialize()
{
	pros::lcd::initialize();
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

void opcontrol()
{
	std::ifstream songfile;
	songfile.open("/usd/song.txt");

	int bpm, fps, fcount;
	songfile >> bpm >> fps >> fcount;

	double frameDuration = (1000.0) / (double)fps;

	int startTime = pros::millis();
	int lastFrame = 0;

	// since the brain can't handle <10 ms delay over ADI ports,
	// we will clock it higher and only send out signals when nessessary
	while (lastFrame < fcount)
	{
		int currentTime = pros::millis();
		double delta = currentTime - startTime;
		int frameID = (int)(delta / frameDuration);

		if (frameID != lastFrame)
		{
			lastFrame = frameID;
			int note;
			songfile >> note;

			bool isStart = false;
			if (note > 64)
			{
				note -= 64;
				isStart = true;
			}

			sendSignal(note, isStart);

			pros::lcd::print(0, "Frame %d Note %d", lastFrame + 1, note);
		}

		// min delay is 2
		pros::delay(2);
	}
}

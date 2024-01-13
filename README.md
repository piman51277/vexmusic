# VEX Music

A video player (with audio) for the VEX v5 Brain. Created in order to play Bad Apple on the Brain.

## Concept

Using PROS, it is possible to load custom images and video onto the screen using the LVGL library that is included with PROS. However, the v5 system has no audio capability.

This project remedies that by using an Arduino as a simple tone generator, and interfacing with it through the 8 ADI ports on the V5 Brain. Details about the protocol can be found below.

## Brain -> Arduino Protocol

The main issue using the ADI ports to communicate is bandwidth. Due to hardware limitations, we are limited to 1 update per 10ms for every port (100Hz). In addition, the scheduler on the Brain makes it nigh impossible to get a consistent delay between updates. Therefore, this protocol is designed to work with variable time between updates, and operate up to 100Hz.

#### Wiring

- A -> Clock
- B -> Note Start
- C -> Note ID bit 1
- D -> Note ID bit 2
- E -> Note ID bit 3
- F -> Note ID bit 4
- G -> Note ID bit 5
- H -> Note ID bit 6

#### Protocol

##### Brain Side

1. The Brain sets ports B-H to indicate a note to be played.
2. The Brain flips the state of port A.

##### Arduino Side

1. The Arduino waits for the state of A to change.
2. The Arduino reads the state of C-H, and converts it to a number from 0 to 59.
3. If B is HIGH, the Arduino inserts a small delay (5ms)

> This is so two consecutive notes can be distinguished. If B is LOW, the Arduino treats the note as a continuation of the previous note.

4. The Arduino plays the note corresponding to the number.

#### Note ID

Musical notes from C1 to B5 are assigned a number from 0 to 59.
Internally, 60 is used to indicate a rest.

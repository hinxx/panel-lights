# Panel-lights

Author: _Hinko Kocevar_, 2020

Two 8x8 panels of RGB LEDs controlled by Arduino board.

This sketch reads LED panel sequence definitions from text files found on
a SD card.

Sequence is divided into steps. In each step both LED panels are either turned
on or off. A step is described by two colors, one for each panel, and duration
to wait.

Any RGB value can be used for LED panel color. RGB value 0x000000 means panels
are turned off, any other value turns the panel on. 

Text file can contain a single sequence. Text files are placed on a SD card, 
in root folder (sub folders are not supported at the moment).

A small 128 x 128 OLED display and rotary encoder with switch button are
used to provide rudimentary user interface. Navigating to a desired filename
in the list of filenames on display, and selecting a file to be loaded causes
the sequence steps to be played out infinitely.

# Hardware

You need:

 * 1x Arduino board (https://store.arduino.cc/arduino-uno-rev3-smd)
 * 2x Adafruit NeoPixel NeoMatrix 8x8 - 64 RGB LED Pixel Matrix (https://www.adafruit.com/product/1487)
 * 1x 128x128 OLED display (https://www.newhavendisplay.com/nhd15128128asc3-p-9287.html)
 * 1x Rotary encoder with switch (https://www.bourns.com/docs/product-datasheets/pec12r.pdf)
 * 1x 5V 3A power adapter (any will do)

Power adapter needs to supply enough current since one panel of 64 LEDs can
use up to 3.8 A. By setting the LED brightness to less than 100 % this can be
reduced and in my application I used 5V/3A adapter with success.

# Software

Sketch uses:

 * modified Adafruit NeoPixel library (https://github.com/adafruit/Adafruit_NeoPixel).
 * Adafruit GFX graphics library (https://github.com/adafruit/Adafruit-GFX-Library)
 * Adafruit SSD1351 OLED display library (https://github.com/adafruit/Adafruit-SSD1351-library)
 * Arduino built-in SPI and SD libraries

Modifications to the Adafruit NeoPixel library were made in order to severly cut
down the memory usage of the sketch. Originally, NeoPixel library allocates 3
bytes per RGB LED of memory; in our case this means

    64 LEDs x 2 panels x 3 bytes = 384 bytes
    
would be used.

Since the our application uses a single color per whole panel (all 64 LEDs are
using the same color) the LED driving inline assembly code was modified to
use only two colors. This resulted in big memory saving. Also, unused functinality
of the NeoPixel library was stripped off to reduce code footprint. 

# Sequences and steps

As mentioned earlier, sequence (list of steps) is stored in a text file. Each
step of a sequence needs to be defined as a string of text. Individual values
can be treated as decimal (DEC) or hexadecimal (HEX) integers by the sketch code.
 
Steps are separated by a newline (`\n`) character.

Step string format is as follows:

    XX AAAAAA YY BBBBBB CC

where

 * XX - panel 1 mode (HEX)
 * AAAAAA - panel 1 color in RGB (HEX)
 * YY - panel 2 mode (HEX)
 * BBBBBB - panel 2 color in RGB (HEX)
 * CC is the duration of the step (DEC)

Mode byte are used to determine the panel color mode as follows

 * 00 - use the supplied color
 * 01 - use random color (ignores supplied color) 

In future mode byte can be used to extend functinality or behavior further.

For color bytes any combination of the values is valid (0 - 255). Values from
1 to 255 result in panel being turned on lit with specified color. A value of
000000 results in panel being turned off.

Duration of the step is supplied in tens of miliseconds; in other words the
sketch will multiply the CC byte with 100 and interpret the result as the amount
of miliseconds to wait before moving on to the next step in sequence. This gives
Maximum duration is therefore 9.9 seconds (99 x 100 ms).

# Host sequence tool

A simple host based tool is supplied to for sequence manipulation.
It allows 

 * listing sequences on a SD card
 * loading a sequence from a SD card
 * editing an existing sequence (by manipulating steps)
 * generating a new sequence
 * removing a sequence
 * simulating sequence run

The tool user interface is based on Dear ImGUI and can be used on any modern 
operating system (OSX, Linux, Windows). 
 
  
# Bugs, improvements, features

If you have found a bug, have an improvement in mind or new feature request
open up and issue or submit a pull request.
 

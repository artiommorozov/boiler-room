## General

Live setup uses Olimex linuxino A20 micro board, should be suitable for other boards with Linux where you have
GPIO and I2C access.

## What's inside

### heating

sources for 'heating' binary. It takes boardConfig.json, periodically polls for weather.json and 
controls hardware

### roomDrawing

drawing of boiler room hardware

### schematics 

KiCad projects with 2 boards (same page) interfacing Olimex board to motors, valves and sensors. 
One (left) is logic-level only and serves connecting bunch of ds18b20 sensors via ds2482 bridge;
it also contains 2 darlington arrays ULN2003 connecting 3.3V GPIO to 5V relays.
The other (right) is relay and feedback board.

### boardCfg

fex file for enabling PE port GPIO on Olimex A20 Micro (google for A20-GPIO.pdf for more) 

## Notes

Code for using DS18b20 via DS2482-800 is for kernel 3.4 that i have on Olimex board. On newer kernel 
one might use kernel module.  
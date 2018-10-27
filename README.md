[Русский][#Коротко]

## General

Live setup uses Olimex linuxino A20 micro board, should be suitable for other boards with Linux where you have
GPIO and I2C access.

## What's inside

### heating

sources for 'heating' binary. It takes config.json, periodically polls for userConfig.json and
controls hardware. See examples in heating/config.samples

Changes in configs are loaded on change, except for outside to mix temp curve settings.

### roomDrawing

drawing of boiler room hardware - TBD

### schematics

KiCad projects with 2 boards (same page) interfacing Olimex board to motors, valves and sensors.
One (left) is logic-level only and serves connecting bunch of ds18b20/ds18s20 sensors via ds2482 bridge;
it also contains 2 darlington arrays ULN2003 connecting 3.3V GPIO to 5V relays.
The other (right) is relay and feedback board.

On a20 micro board use gpio pc3 for boiler sense if you don't want external pullup, other pins
selection is not limited.

### boardCfg

fex file for enabling PE port GPIO on Olimex A20 Micro
[How to use](http://linux-sunxi.org/GPIO)

## Notes

Code for using DS18b20 via DS2482-800 is for kernel 3.4 that i have on Olimex board. On newer kernel
one might use kernel module.

## Коротко

Моя котельная построена на плате Olimex linuxino A20 micro, в принципе этот код должен быть
совместим с другими платами с OS Linux где есть I2C и GPIO.

## Внутри

### heating

Исходники для бинарника heating. Загружает config.json и userConfig.json и управляет устройствами.
Настройки (кроме кривой отопления) подгружаются по мере изменений. См. примеры в heating/config.samples

### roomDrawing

Схема котельной - доделаю скоро

### schematics

Проект в KiCad для плат, соединяющих плату Olimex с моторами, клапанами и сенсорами.
Левая часть - плата логики, соединяет десяток ds18b20/ds18s20 через ds2482. Там же
стоит 2 сборки Дарлингтона uln2003 для управления 5В реле от 3.3В GPIO.

Правая часть - силовая - реле и обратная связь привода крана температуры.

На плате Olimex А20 micro для бойлера лучше использовать pc3 (без pullup резистора). Остальные
GPIO можно выбирать по вкусу =)

### boardCfg

fex для включения GPIO порта pe и pc3 на Olimex A20 micro.
[Как пользоваться](http://linux-sunxi.org/GPIO)

## Заметки

На новых ядрах есть драйвер для ds2482, код может быть проще. На А20 micro доступно только старое ядро
3.4

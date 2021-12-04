[Русский](#ru)

## General

Live setup uses Olimex linuxino A20 micro board, should be suitable for other boards with Linux where you have
GPIO and I2C access.

## What's inside

### heating

sources for 'heating' binary. It takes config.json, periodically polls for userConfig.json and
controls hardware. See examples in heating/config.samples

Changes in configs are loaded on change.

### roomDrawing

drawing of boiler room hardware with matching parts in config.json

### schematics

KiCad projects with 2 boards (same page) interfacing Olimex board to motors, valves and sensors.
One (left) is logic-level only and serves connecting bunch of ds18b20/ds18s20 sensors via ds2482 bridge;
it also contains 3 darlington arrays ULN2003 connecting 3.3V GPIO to 5V relays.
The other (right) is relay and feedback board.

On a20 micro board use gpio pc3 for boiler sense if you don't want external pullup, other pins
selection is not limited.

### boardCfg

fex file for enabling PE port GPIO on Olimex A20 Micro
[How to use](http://linux-sunxi.org/GPIO)

### htmlGraph

Operation history (TODO) and temperature sensors history via browser. Web server on board should expose 
this folder along with config.json as /config/config.json and log files as per config.json "web" section.
Index for logging directories should be turned on.

Data filters on page are persistent.

### testcpp 

test project with stubs to allow running heating binary on windows under MSVC2017

### weather

You can use any source for outside temperature and update userConfig.json. My selection is 'feels like'
section of local weather forecast service.

## Notes

Code for using DS18b20 via DS2482-800 is for kernel 3.4 that i have on Olimex board. On newer kernel
one might use kernel module.


### ru
## Коротко

Моя котельная построена на плате Olimex linuxino A20 micro, в принципе этот код должен быть
совместим с другими платами с OS Linux где есть I2C и GPIO.

## Внутри

### heating

Исходники для бинарника heating. Загружает config.json и userConfig.json и управляет устройствами.
Настройки подгружаются по мере изменений. См. примеры в heating/config.samples

### roomDrawing

Схема котельной, показывает расположение датчиков и элементов управления из config.json

### schematics

Проект в KiCad для плат, соединяющих плату Olimex с моторами, клапанами и сенсорами.
Левая часть - плата логики, соединяет десяток ds18b20/ds18s20 через ds2482. Там же
стоит 3 сборки Дарлингтона uln2003 для управления 5В реле от 3.3В GPIO.

Правая часть - силовая - реле и обратная связь привода крана температуры.

На плате Olimex А20 micro для бойлера лучше использовать pc3 (без pullup резистора). Остальные
GPIO можно выбирать по вкусу =)

### boardCfg

fex для включения GPIO порта pe и pc3 на Olimex A20 micro.
[Как пользоваться](http://linux-sunxi.org/GPIO)

### htmlGraph

История управления (TODO) и история показаний датчиков в браузере. Веб сервер на плате должен 
отдавать эту директорию, config.json как /config/config.json и директории логов согласно
разделу "web" в config.json. Индекс для директорий логов должен быть включен.

Фильтр данных сохраняется при перезагрузке страницы.

### testcpp 

тестовый проект с заглушками, чтобы отлаживать heating в MSVC2017

### weather

Источник погоды для userConfig.json подойдет любой, мне нравится сайт прогноза погоды, раздел "ощущается как".

## Заметки

На новых ядрах есть драйвер для ds2482, код может быть проще. На А20 micro доступно только старое ядро
3.4


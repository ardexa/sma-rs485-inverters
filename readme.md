
# Purpose
The purpose of this project is to collect from SMA solar inverters using an RS485 connection and send the data to your cloud using Ardexa. Data from SMA solar inverters is read using a Linux device such as a Raspberry Pi, or an X86 intel powered computer. 

## How does it work
This application is written in C++, using the SMA provided C libraries to query SMA inverters connected via RS485. This application will run as a service, and query any number of connected inverters at regular intervals. Data will be written to log files on disk in a directory specified via the command line. Usage and command line parameters are as follows. Note that the applications should be run as root only since it has access to a device in the `/dev` directory.

Usage: sudo ardexa-sma -c conf file path -n number of devices [-l log directory] [-d] [-v] [-i] [-s number of seconds between readings]
```
-l (optional) <directory> name for the location of the directory in which the logs will be written. The default is `/opt/ardexa/sma/logs`
-c (mandatory) <file path> fullpath of the SMA config file. An explanation of the config file is below.
-d (optional) if specified, debug will be turned on. Default is off.
-i (optional) discovery. Print (and if debug is on, send to the console) a listing of all available objects and variables on all inverters.
-v (optional) prints the version and exits.
-s (optional) delay between readings. Default is 60 seconds. Ignored during discovery (-i option).
-n (mandatory) number of devices to find. Must be at least 1, and less than 40.
```

The only 2 mandatory items are the number of inverters (`-n`) and the configuration file `-c`. So an example of the usage is : `sudo ardexa-sma -c /home/ardexa/yasdi.conf -n 1`
So for example, to run a discovery:
	sudo ardexa-sma -c yasdi.conf -n 1 -i

## RS485 to USB converter
The SMA (as most inverters) can use RS485 as a means to communicate data and settings
RS485 is a signalling protocol that allows many devices to share the same physical pair of wires, in a master master/slave relationship
See -> http://www.usb-serial-adapter.org/ for further information

When an RS485 to USB converter has been plugged in, on Linux systems the device will connect to something line /dev/ttyUSB0. To check:
```
sudo tail -f /var/log/syslog
...then plug in the converter
```
You should see a line like: `usb 1-1.4: ch341-uart converter now attached to ttyUSB0`
This means that the RS485 serial port can be accessed by the logical device `/dev/ttyUSB0`
Alternatively, try: `dmesg | grep tty`

## Inverter to RS485 (DB9) Physical Connection
Reference for the wiring:
- https://support.tigoenergy.com/hc/en-us/articles/200337847-Monitor-SMA-Inverter-via-RS485

If your computer has an RS485 port, then the inverter can be connected directly to this port.
The inverter is connected using 3 wires to the RS485 DB9 port on the computer. DO NOT connect the inverter RS485 to a RS232 port. They are not voltage compatible and damage will probably occur. For this to happen, you need a devices like these:
- http://www.robotshop.com/en/db9-female-breakout-board.html
- https://core-electronics.com.au/db9-female-breakout-board.html
- https://www.amazon.com/Female-Breakout-Board-Screw-Terminals/dp/B00CMC1XFU

Each and every RS485 port that uses DB9 has a different pinout. So you have to read the actual manual for the physical RS485 port you are using. 
For example; if using the Advantech UNO 2362G, the following pins are used: Pin1 = D- , Pin 2 = D+ and Pin 5 = GND. All other pins are not connected, so do not connect any other pins. 
So to wire it all up:
- Make sure the Advantech is turned off
- D+ Pin from the SMA Inverter (should be Pin 2 on the RS485 inverter interface) to Pin 2 of the RS485 DB-9 (Female) on the Advantech UNO 2362G
- GND Pin from the SMA Inverter (should be Pin 5 on the RS485 inverter interface) to Pin 5 of the RS485 DB-9 (Female) on the Advantech UNO 2362G
- D- Pin from the SMA Inverter (should be Pin 7 on the RS485 inverter interface) to Pin 1 of the RS485 DB-9 (Female) on the Advantech UNO 2362G

Confirm the physical serial port by running the command `dmesg | grep tty`. As stated previously, it should return something like `/dev/ttyS1` if using a serial com port, or something like `/dev/ttyUSB0` if using a 485/USB serial converter.


## Building SMA YASDI Library
The YASDI software (See: http://www.sma.de/en/products/monitoring-control/yasdi.html) allows communications with the SMA inverter via RS485. Download thre ZIP file and do the following to install the libraries and utlities.
You may need to install all the build tools as follows
```
sudo apt-get update
sudo apt-get install -y build-essential git cmake
```

then install the YASDI software
```
cd
mkdir sma
cd sma
...from http://www.sma.de/en/products/monitoring-control/yasdi.html#Downloads
wget yasdi-1.8.1build9-src.zip
unzip yasdi-1.8.1build9-src.zip
edit the file ../yasdi/include/packet.h and change Line 38: from struct TDevice * Device ...to.... struct _TDevice * Device
cd projects/generic-cmake
mkdir build-gcc
cd build-gcc
cmake ..
make
sudo make install     
sudo ldconfig
cd
vi yasdi.conf
	[DriverModules]
	Driver0=yasdi_drv_serial

	[COM1]
	Device=/dev/ttyUSB0
	Media=RS485
	Baudrate=1200
	Protocol=SMANet

sudo yasdishell yasdi.conf ... just to test
```

In the 'yasdi.conf' file above, change the 'Device=...' to whatever the device name is, as per the instructions above.

## Building the Ardexa software
- Make sure the YASDI application is in the directory above `../sma/`
```
cd
git clone https://github.com/ardexa/sma-rs485-inverters.git
cd sma-rs485-inverters
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
```

## Installing as a Service
The Ardexa service will query one or all of the inverters. To query at regular intervals, and write a message to the log, a service is required. The following instructions detail how to install the application to run as a service. The attached `ardexa-sma.service` file is used to run the application as a service. Edit the line `ExecStart=/usr/local/bin/ardexa-sma -c /home/ardexa/yasdi.conf -n 13 -s 300` to change the number of inverters that will be searched (the `-n 13` parameter) and the time between readings (the `-s 300` parameter). 

```
sudo cp ardexa-sma.service /etc/systemd/system/
sudo systemctl enable ardexa-sma.service
sudo systemctl daemon-reload
sudo systemctl restart ardexa-sma.service
```

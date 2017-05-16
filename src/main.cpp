/* 
* Copyright (c) 2013-2017 Ardexa Pty Ltd
*
* This code is licensed under the MIT License (MIT).
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
*/


#ifdef __cplusplus
extern "C" {
#endif

#include "libyasdi.h"
#include "libyasdimaster.h"
#include "tools.h"

#ifdef __cplusplus
}
#endif

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <ctime>
#include <map>
#include "utils.hpp"
#include "arguments.hpp"


#define DEVICE_MAX 50
#define NAME 64
#define MAXDRIVERS 10   
#define MAX_CHANNEL_COUNT 500

using namespace std;

/* Global variables. */ 
int g_debug = DEFAULT_DEBUG_VALUE;


/* function prototypes */
bool detect_devices( int device_count);
void record_devices(map <DWORD, string> &device_map, bool discovery);
bool fetch_dynamic_data(DWORD device_handle, string *header_out, string *data_out, bool discovery, arguments arguments_list);
string list_texts(DWORD channel_handle, string channel_name);


/************************ Functions start ******************************/

/* This function will detect devices */
bool detect_devices( int device_count)
{
   int error;
   
   if (g_debug > 0) {
		cout << "Trying to detect the following number of devices: " << device_count << endl;
	}
   
   /* Blocking call to detect devices */
   error = DoStartDeviceDetection(device_count, TRUE);
   switch(error) {
		case YE_OK:
			return true; 

      case YE_DEV_DETECT_IN_PROGRESS:
         cout << "Error: Detection in progress" << endl;
         return false; 
      
      case YE_NOT_ALL_DEVS_FOUND:
         cout << "Error: Not all devices were found" << endl;
         return false;
         
      default:
         cout << "Error: Unknown YASDI error" << endl;
         return false;          
   }
}


/* Record all devices to a vector, and (if discovery or debug is on) print the list of devices */
void record_devices(map <DWORD, string> &device_map, bool discovery)
{
   DWORD handles_array[DEVICE_MAX], device, count = -1;
   char namebuf[NAME] = "";

	/* Clear the map */
	device_map.clear();

   /* get all device handles...*/
   count   = GetDeviceHandles(handles_array, DEVICE_MAX );
   if (count > 0) {
      for (device=0; device < count ; device++) {
         /* get the name of this device */
         GetDeviceName(handles_array[device], namebuf, sizeof(namebuf)-1);
         if ((g_debug) or (discovery)) cout << "Found device with a handle of : " << handles_array[device] << " and a name of: " << namebuf << "\n" << endl;
			string device_raw = namebuf;
			string device_name = replace_spaces(device_raw);
			/* Add it to the map */
			device_map[handles_array[device]] = device_name;
      }
   }
	else {
		if ((g_debug) or (discovery))cout << "No devices have been found" << endl;
   }
}


/* This function will retrieve the channel and header data as a comma separated list
	If 'discovery' or 'debug' is listed as true, it will also print the values   
 */
bool fetch_dynamic_data(DWORD device_handle, string *header_out, string *data_out, bool discovery, arguments arguments_list)
{
   DWORD channel_array[MAX_CHANNEL_COUNT];
	int channel_count = -1;
	char channel_name[NAME];
	char channel_units[NAME];
	char channel_value[NAME];
	string header_entry;
	int result;
	vector <string> data;
	vector <string> header;
	string channel_value_str;
   double double_val = 0;
	/* maximum age of the channel value in seconds. If the age of the data (as determined by the SMA inverter) is greater than this, then 
	   query the inverter to get the lastest data. If it can't log a line, it returns a FALSE */
   DWORD max_age = 5; 

   channel_count = GetChannelHandlesEx(device_handle, channel_array, MAX_CHANNEL_COUNT, SPOTCHANNELS);
	if (channel_count < 1) {
		if (g_debug) cout << "Could not get the channel count" << endl;
		return false;
	}

	/* get the datetime */
	data.push_back(get_current_datetime());
	header.push_back("datetime");

	/* Print each of the channel values */
   for(int i = 0; i < channel_count; i++) {
		channel_value_str = "";
		result = GetChannelName(channel_array[i], channel_name, sizeof(channel_name)-1);
		if(result == YE_OK) {
			/* also get the units of the readings type ..eg; kWh, V, etc */
			GetChannelUnit(channel_array[i], channel_units, sizeof(channel_units)-1);
			string unit_str(channel_units);
			string name_str(channel_name);
			if (arguments_list.convert.find(name_str) != arguments_list.convert.end()) {
				name_str = arguments_list.convert[name_str];
			}
			header_entry = name_str + "(" + unit_str + ")";	
		}
		else {
			/* If a channel cannot be read, then exit */
			if (g_debug) cout << "Error reading channel value for channel: " << channel_name << endl;
         return false;
		}
		header.push_back (header_entry); 

  	   /* And get the channel value. If a channel cannot be read, then exit */
      result = GetChannelValue(channel_array[i], device_handle, &double_val, channel_value, sizeof(channel_value)-1, max_age);
      if(result == YE_OK) {
         /* Convert the value to a string. Do not use 'to_string'. Does not work well at all */
			channel_value_str = channel_value;
			if (channel_value_str.empty()) {
				channel_value_str = convert_double(double_val);
			}
			else {
				if (arguments_list.convert.find(channel_value_str) != arguments_list.convert.end()) {
					channel_value_str = arguments_list.convert[channel_value_str];
				}
			}
      }
      else {
			if (g_debug) cout << "Error reading channel value for channel: " << channel_name << endl;
         return false;
      }
		data.push_back(channel_value_str);
		if (discovery) {
			list_texts(channel_array[i], header_entry);
		}
	}

	int i = 0;
	string data_line = "";
	for(vector<string>::iterator it = data.begin() ; it < data.end(); it++, i++ ) {
		if (i == 0) data_line += *it;
		else data_line += "," + *it;
	}

	i = 0;
	string header_line = "";
	for(vector<string>::iterator it = header.begin() ; it < header.end(); it++, i++ ) {
		if (i == 0) header_line += *it;
		else header_line += "," + *it;
	}

	if (g_debug) cout << "Header line: " << header_line << endl;
	if (g_debug) cout << "Data line: " << data_line << endl;
	*header_out = header_line;
	*data_out = data_line;

	return true;
}


/* This function will get all status texts associated with a channel. It is used for data discovery */
string list_texts(DWORD channel_handle, string channel_name)
{
   int i;
   int text_count = GetChannelStatTextCnt(channel_handle);
	string output;
   if (text_count) {
		if (g_debug) cout << "Channel name has the following texts (these names are raw from the SMA device): " << endl;
		for(i=0; i < text_count; i++) {
         char stat_text[NAME];
         GetChannelStatText(channel_handle, i, stat_text, sizeof(stat_text)-1);
			cout << "\tChannel name: " << channel_name << " has the text: " << stat_text << "\n";
      }
   }
	
	return output;
}


/************************ Functions end ******************************/


/* This is the main function */
int main(int argc, char *argv[])
{
	DWORD drivers = 0;
	int result = 0;
	char DriverName[NAME];
	bool any_driver = false;
   DWORD Driver[MAXDRIVERS]; 
	map <DWORD, string> device_map;

	/* If not run as root, exit */
	if (check_root() == false) {
		cout << "This program must be run as root" << endl;
		return 1;
	}

	/* Check for existence of PID file */
	if (!check_pid_file()) {
		return 2;
	}

	/* This class object defines the initial configuration parameters */     
	arguments arguments_list;     
	result = arguments_list.initialize(argc, argv);     
	if (result != 0) {         
		cerr << "Incorrect arguments" << endl;         
		return 3;     
	}     

	/* set the global debug value. this won't change during runtime */     
	g_debug = arguments_list.get_debug();

	string conf_file = arguments_list.get_config_file();
   /* init Yasdi- and Yasdi-Master-Library */
   yasdiMasterInitialize(conf_file.c_str(), &drivers);
   /* get List of all supported drivers...*/
   drivers = yasdiMasterGetDriver(Driver, MAXDRIVERS );
   /* Switch all drivers online */
   for(DWORD i=0; i < drivers; i++) {
		if (g_debug) {
			/* The name of the driver */
   	   yasdiGetDriverName(Driver[i], DriverName, sizeof(DriverName) - 1);
			cout << "Switching on driver: " << DriverName << endl;
		}

      if (yasdiSetDriverOnline(Driver[i])) {
         any_driver = true;
      }
   }
   
   /* Check that at least one driver is online */
   if (any_driver == false) {
		cout << "No drivers found. Exitting. " << endl;
		return 4;
	}

	/* If not all devices are found, then we will try again later */
	bool all_devices_found = detect_devices(arguments_list.get_number());
	record_devices(device_map, arguments_list.get_discovery());

	bool run = true;
	if (arguments_list.get_discovery()) run = false;

	/* A few things about this loop:
	   1. If discovery has been set, it will print one set of values for all devices found and then exit
		2. If not all devices have been found, then retry to find them every 10 minutes or so
	*/
	string current_date = get_current_date();
	string previous_date = get_current_date();
	int running_total = 0;
	bool success_read = false;
	do {
		string data, header;
		string current_date = get_current_date();
		time_t start = time(nullptr);
		for(map<DWORD, string>::const_iterator it = device_map.begin(); it != device_map.end(); ++it) {
			success_read = fetch_dynamic_data(it->first, &header, &data, arguments_list.get_discovery(), arguments_list);
			/* If this is a discovery query, then print data and exit */
			if (arguments_list.get_discovery()) {
				cout << "Data: " << data << endl;
				cout << "Header: " << header << endl;
				run = false;			
			}
			else {
				/* Only log a line if it was a success */
				if (success_read) {
					/* Log the line based on the inverter name, in the logging directory */
					string full_dir = arguments_list.get_log_directory() + "/" + it->second;
					/* log to a date and to a 'latest' file */
					log_line(full_dir, current_date + ".csv", data, header, true);
				}
			}
		}
		time_t end = time(nullptr);
		if (g_debug) cout << "Query took: " << (end-start) << " Seconds\n" << endl;
		previous_date = current_date;
		/* If the loop will run continuously, then run a delay */
		if (run) sleep(arguments_list.get_delay());

		/* if not all devices have been found, then try to find them at least once every 20 minutes */
		if ((not all_devices_found) and (run)) {
			/* Add a running total. If it is greater than 20 minutes, redo 'detect_devices' and 'record_devices' */
			running_total += arguments_list.get_delay();
			if (running_total > 1200) {
				if (g_debug) cout << "Not all devices were found in the original run, trying to find them now" << endl;
				all_devices_found = detect_devices(arguments_list.get_number());
				record_devices(device_map, false);
				running_total = 0;
			}
		}

	} while (run);


   /* Shutdown all yasdi drivers... */
   for(DWORD i=0; i < drivers; i++) {
      yasdiGetDriverName(Driver[i], DriverName, sizeof(DriverName));
      yasdiSetDriverOffline( Driver[i] );
   }

   /* Shutdown YASDI */
   yasdiMasterShutdown();

	remove_pid_file();
   return 0;
}

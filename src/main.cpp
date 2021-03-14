/*
 * Copyright (c) 2013-2018 Ardexa Pty Ltd
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

#undef min 
#undef max

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <ctime>
#include <map>
#include <sstream>
#include <iomanip>
#include "utils.hpp"
#include "arguments.hpp"


#define DEVICE_MAX 50
#define SIZE_NAME 64
#define MAXDRIVERS 10
#define MAX_CHANNEL_COUNT 500

using namespace std;

/* Global variables. */
int g_debug = DEFAULT_DEBUG_VALUE;

/* This is used by the vector to store all the data that is collected */
struct vec_data {
    string name;
    string name_units;
    string value;
};


/* function prototypes */
bool detect_devices( int device_count);
void record_devices(map <DWORD, string> &device_map, bool discovery);
bool fetch_dynamic_data(DWORD device_handle, string *header_out, string *data_out, bool discovery, arguments arguments_list);
string list_texts(DWORD channel_handle, string channel_name);
void process_data(vector <vec_data> data_vector, int debug, string& line, string& header);

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
    char namebuf[SIZE_NAME] = "";

    /* Clear the map */
    device_map.clear();

    /* get all device handles...*/
    count   = GetDeviceHandles(handles_array, DEVICE_MAX);
    if (count > 0) {
        for (device=0; device < count ; device++) {
            /* get the name of this device */
            GetDeviceName(handles_array[device], namebuf, sizeof(namebuf)-1);
            if ((g_debug) or (discovery)) cout << "Found device with a handle of : " << handles_array[device] << " and a name of: " << namebuf << "\n" << endl;
            string device_raw = string(namebuf);
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
    char channel_name[SIZE_NAME];
    char channel_units[SIZE_NAME];
    char channel_value[SIZE_NAME];
    string header_entry;
    int result;
    string channel_value_str;
    double double_val = 0;
    vector <vec_data> data_vector;
    /* maximum age of the channel value in seconds. If the age of the data (as determined by the SMA inverter) is greater than this, then
       query the inverter to get the lastest data. If it can't log a line, it returns a FALSE */
    DWORD max_age = 5;

    channel_count = GetChannelHandlesEx(device_handle, channel_array, MAX_CHANNEL_COUNT, SPOTCHANNELS);
    if (channel_count < 1) {
        if (g_debug) cout << "Could not get the channel count" << endl;
        return false;
    }

    /* Print each of the channel values */
    for(int i = 0; i < channel_count; i++) {
        vec_data vec_data_temp = { { 0 } };
        channel_value_str = "";
        string unit_str;
        string name_str;

        result = GetChannelName(channel_array[i], channel_name, sizeof(channel_name)-1);
        if(result == YE_OK) {
            /* also get the units of the readings type ..eg; kWh, V, etc */
            GetChannelUnit(channel_array[i], channel_units, sizeof(channel_units)-1);
            unit_str = channel_units;
            name_str = channel_name;
            if (arguments_list.convert.find(name_str) != arguments_list.convert.end()) {
                name_str = arguments_list.convert[name_str];
            }

        }
        else {
            /* If a channel cannot be read, then continue */
            if (g_debug) cout << "Error reading channel value for channel: " << channel_name << endl;
            continue;
        }

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
            continue;
        }


        vec_data_temp.name = name_str;
        vec_data_temp.name_units = name_str + "(" + unit_str + ")";
        vec_data_temp.value = channel_value_str;
        data_vector.push_back(vec_data_temp);

        if (discovery) {
            list_texts(channel_array[i], header_entry);
        }
    }

    if (discovery) {
        for (auto iter = data_vector.begin(); iter != data_vector.end(); ++iter) {
            string value = iter->value;
            string name = iter->name;
            string name_units = iter->name_units;
            cout << "Name: " << name << " name(units): " << name_units << " and value: " << value << endl; ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }
    }

    string data_line = "";
    string header_line = "";
    process_data(data_vector, g_debug, data_line, header_line);

    *header_out = header_line;
    *data_out = data_line;

    ////*************************************************************************
    ////*********************	int i = 0;
    ////*********************	string data_line = "";
    ////*********************	for(vector<string>::iterator it = data.begin() ; it < data.end(); it++, i++ ) {
    ////*********************		if (i == 0) data_line += *it;
    ////*********************		else data_line += "," + *it;
    ////*********************	}

    ////*********************	i = 0;
    ////*********************	string header_line = "";
    ////*********************	for(vector<string>::iterator it = header.begin() ; it < header.end(); it++, i++ ) {
    ////*********************		if (i == 0) header_line += *it;
    ////*********************		else header_line += "," + *it;
    ////*********************	}

    ////*********************	if (g_debug) cout << "Header line: " << header_line << endl;
    ////*********************	if (g_debug) cout << "Data line: " << data_line << endl;
    ////*********************	*header_out = header_line;
    ////*********************	*data_out = data_line;


    return true;
}


/* This function will get all status texts associated with a channel. It is used for data discovery */
string list_texts(DWORD channel_handle, string channel_name)
{
    int i;
    int text_count = GetChannelStatTextCnt(channel_handle);
    string output;
    if (text_count) {
        cout << "Channel name has the following text options (these names are raw from the SMA device): " << endl;
        for(i=0; i < text_count; i++) {
            char stat_text[SIZE_NAME];
            GetChannelStatText(channel_handle, i, stat_text, sizeof(stat_text)-1);
            cout << "\tChannel name: " << channel_name << " has the text: " << stat_text << "\n";
        }
    }

    return output;
}



/* This function processes the data that is in a vector of structs */
void process_data(vector <vec_data> data_vector, int debug, string& line, string& header)
{
    string pac_header, yield_header, pdc1_header, pdc2_header, vdc1_header, vdc2_header, vac1_header, vac2_header, vac3_header, iac1_header, iac2_header, iac3_header, idc1_header, idc2_header;
    string pac1_header, pac2_header, pac3_header, gridfreq_header, cosphi_header, mode_header, error_header, op_hours_header, isol_header;
    string pac_str, yield_str, pdc1_str, pdc2_str, vdc1_str, vdc2_str, vac1_str, vac2_str, vac3_str, iac1_str, iac2_str, iac3_str, idc1_str, idc2_str;
    string pac1_str, pac2_str, pac3_str, gridfreq_str , cosphi_str, mode_str, error_str, op_hours_str, isol_str;

    

    /* For a 3 Phase inverter, these are the values we need ....
       A.Ms.Amp					String A Current	decimal		A			eg; 2.84
       B.Ms.Amp					String B Current	decimal		A			eg; 0.93
       A.Ms.Vol					String A Voltage	decimal		V			eg; 455.40
       B.Ms.Vol					String B Voltage	decimal		V			eg; 445.37
       A.Ms.Watt				String A Power 	    integer		W			eg; 1295
       B.Ms.Watt				String B Power 	    integer		W			eg; 415
       grid power				AC Power			integer		W			eg; 1635
       GridMs.W.phsA			AC Power 1			integer		W			eg; 545
       GridMs.W.phsB			AC Power 2			integer		W			eg; 545
       GridMs.W.phsC			AC Power 3			integer		W			eg; 545
       GridMs.PhV.phsA		    AC Voltage 1		decimal		V			eg; 234.82
       GridMs.PhV.phsB		    AC Voltage 2		decimal		V			eg; 234.99
       GridMs.PhV.phsC		    AC Voltage 3		decimal		V			eg; 236.12
       GridMs.A.phsA			AC Current 1		decimal		A			eg; 2.32
       GridMs.A.phsB			AC Current 2		decimal		A			eg; 2.32
       GridMs.A.phsC			AC Current 3		decimal		A			eg; 2.31
       GridMs.Hz				Grid Freq			decimal		Hz			eg; 50
       GridMs.TotPF			    Cos Phi				keyword					eg; 1 ... beware, this may not be a number
       energy yield			    Energy Yield		decimal		kWh		    eg; 43483.59   ###################################### check this is converted to Wh #########################
       Mode						Status				keyword					eg; Mpp
       Error				    Error				keyword					eg; ok
       Operating Hours          OPerating Hours     decimal     h           eg; 1234.5
       Isolation Resistance     Iso Resist          decimal     kOhm        eg; 3000
       

       */


    for (auto iter = data_vector.begin(); iter != data_vector.end(); ++iter) {
        string value = iter->value;
        string name_units = iter->name_units;
        string name = iter->name;

        if ((name == "A.Ms.Amp") or (name == "pv panels current")) {
            idc1_header = name_units;
            idc1_str = value;
        }

        if (name == "B.Ms.Amp") {
            idc2_header = name_units;
            idc2_str = value;
        }

        if ((name == "A.Ms.Vol") or (name == "pv input voltage")) {
            vdc1_header = name_units;
            vdc1_str = value;
        }

        if (name == "B.Ms.Vol") {
            vdc2_header = name_units;
            vdc2_str = value;
        }

        if (name == "A.Ms.Watt") {
            pdc1_header = name_units;
            pdc1_str = value;
        }

        if (name == "B.Ms.Watt") {
            pdc2_header = name_units;
            pdc2_str = value;
        }

        if (name == "grid power") {
            pac_header = name_units;
            pac_str = value;
        }

        if (name == "GridMs.W.phsA") {
            pac1_header = name_units;
            pac1_str = value;
        }

        if (name == "GridMs.W.phsB") {
            pac2_header = name_units;
            pac2_str = value;
        }

        if (name == "GridMs.W.phsC") {
            pac3_header = name_units;
            pac3_str = value;
        }

        if ((name == "GridMs.PhV.phsA") or (name == "grid voltage")) {
            vac1_header = name_units;
            vac1_str = value;
        }

        if (name == "GridMs.PhV.phsB") {
            vac2_header = name_units;
            vac2_str = value;
        }

        if (name == "GridMs.PhV.phsC") {
            vac3_header = name_units;
            vac3_str = value;
        }

        if ((name == "GridMs.A.phsA") or (name == "current to grid")) {
            iac1_header = name_units;
            iac1_str = value;
        }

        if (name == "GridMs.A.phsB") {
            iac2_header = name_units;
            iac2_str = value;
        }

        if (name == "GridMs.A.phsC") {
            iac3_header = name_units;
            iac3_str = value;
        }

        if (name == "energy yield") {
            yield_header = name_units;
            yield_str = value;
        }

        if ((name == "GridMs.Hz") or (name == "grid freq")) {
            gridfreq_header = name_units;
            gridfreq_str = value;
        }

        if (name == "GridMs.TotPF") {
            cosphi_header = name_units;
            cosphi_str = value;
        }

        if ((name == "Mode") or (name == "Status")) {
            mode_header = name_units;
            mode_str = value;
        }

        if ((name == "Error") or (name == "error")) {
            error_header = name_units;
            error_str = value;
        }

        if ((name == "total operating hours")) {
            op_hours_header = name_units;
            op_hours_str = value;
        }

        if ((name == "isol-resist")) {
            isol_header = name_units;
            isol_str = value;

            // Convert "isol_str" to a float
            size_t idx;
            float resist;
            try {
                resist = stof(isol_str, &idx);
            }
            catch ( const std::exception& e ) {
                cout << "Could not convert isol_str value to a float: " << isol_str << endl;
            }
            resist = resist/1000;
            stringstream stream;
            stream << fixed << setprecision(2) << resist;
            isol_str = stream.str();
        }


        string datetime = get_current_datetime();
        header = "#Datetime," + pac_header + "," + yield_header + "," + pdc1_header  + "," + pdc2_header + "," + vdc1_header + "," +
            vdc2_header + "," + vac1_header + "," + vac2_header + "," + vac3_header + "," + iac1_header + "," + iac2_header + "," + iac3_header + "," +
            idc1_header + "," + idc2_header + "," + pac1_header + "," + pac2_header + "," + pac3_header + "," + gridfreq_header + "," + cosphi_header + "," + 
            mode_header + "," + error_header + "," + op_hours_header + "," + isol_header;

        line = datetime + "," + pac_str + "," + yield_str + "," + pdc1_str + "," + pdc2_str + "," + vdc1_str + "," + vdc2_str + "," +
            vac1_str + "," + vac2_str + "," + vac3_str + "," + iac1_str + "," + iac2_str + "," + iac3_str + "," +
            idc1_str + "," + idc2_str + "," + pac1_str + "," + pac2_str + "," + pac3_str + "," + gridfreq_str + "," + cosphi_str + "," + mode_str + "," + 
            error_str + "," + op_hours_str + "," + isol_str;


        if (debug >= 1) {
            cout << "Header: " << header << endl;
            cout << "Line: " << line << endl;
        }

    }

    return;
}




/************************ Functions end ******************************/


/* This is the main function */
int main(int argc, char *argv[])
{
    DWORD drivers = 0;
    int result = 0;
    char DriverName[SIZE_NAME];
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
                if (success_read && !data.empty()) {
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

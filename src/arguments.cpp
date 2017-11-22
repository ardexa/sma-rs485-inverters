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

#include "arguments.hpp"
#include "utils.hpp"

using namespace std;

/* Constructor for the arguments class */
arguments::arguments()
{
    /* Initialize the private members */
    this->debug = DEFAULT_DEBUG_VALUE;
    this->log_directory = DEFAULT_LOG_DIRECTORY;
    this->discovery = false;
    this->conf_filepath = DEFAULT_CONF_DIRECTORY;
    this->number = 0;
    initialise_conversions();

    /* Usage string */
    this->usage_string = "Usage: ardexa-sma -c conf file path -n number of devices [-l log directory] [-d] [-v] [-i] [-s number of seconds between readings]\n";
}

/* This method is to initialize the member variables based on the command line arguments */
int arguments::initialize(int argc, char * argv[])
{
    int opt;
    bool ret_error = false;
    string delay_raw = to_string(DELAY);
    string number_raw = "0";

    /*
     * -l (optional) <directory> name for the location of the directory in which the logs will be written
     * -c (mandatory) <file path> fullpath of the SMA config file
     * -d (optional) if specified, debug will be turned on
     * -i (optional) discovery. Print (and if debug is on, send to the console) a listing of all available objects and variables
     * -v (optional) prints the version and exits
     * -s (optional) delay between readings. Default is 60 seconds. Ignored during discovery
     * -n (mandatory) number of devices to find. Must be at least 1, and less than 40
     */
    while ((opt = getopt(argc, argv, "l:c:s:n:div")) != -1) {
        switch (opt) {
            case 'c':
                /* verify the existence of the configuration file done below */
                this->conf_filepath = optarg;
                break;
            case 'l':
                /* There is no need to verify the logging directory. If it doesn't exist, it will be created later */
                this->log_directory = optarg;
                break;
            case 's':
                /* verify the delay later below */
                delay_raw = optarg;
                break;
            case 'n':
                /* verify the number of devices later below */
                number_raw = optarg;
                break;
            case 'd':
                this->debug = true;
                break;
            case 'i':
                this->discovery = true;
                break;
            case 'v':
                cout << "Ardexa RS485 SMA Version: " << VERSION << endl;
                exit(0);
            default:
                this->usage();
                return 1;
        }
    }


    /* Convert "number_raw" to INT */
    size_t idx;
    try {
        this->number = stoi(number_raw,&idx,10);
    }
    catch ( const std::exception& e ) {
        ret_error = true;
    }

    if ((this->number <1) or (this->number >= 40)) {
        cout << "Number of devices must be a number, and be greater than 0 and less than 40 " << endl;
        ret_error = true;
    }

    /* Convert "delay_raw" to INT */
    try {
        this->delay = stoi(delay_raw,&idx,10);
    }
    catch ( const std::exception& e ) {
        ret_error = true;
    }

    if (this->delay < 5) {
        cout << "Delay must be a number, and be greater than 5 seconds " << endl;
        ret_error = true;
    }


    g_debug = this->debug;

    if (not create_directory(this->log_directory)) {
        cout << "Could not create the logging directory: " << this->log_directory << endl;
        ret_error = true;
    }

    /* check existance of SMA config file */
    if (check_file(this->conf_filepath) == false) {
        cout << "Config file does not exist" << endl;
        ret_error = true;
    }

    /* If any errors, then return as an error */
    if (ret_error) {
        this->usage();
        return 1;
    }
    else {
        return 0;
    }
}

/* Print usage string */
void arguments::usage()
{
    cout << usage_string;
}

/* Get debug value */
bool arguments::get_debug()
{
    return this->debug;
}

/* Get the logging directory */
string arguments::get_log_directory()
{
    return this->log_directory;
}

/* Get the discovery bool value */
bool arguments::get_discovery()
{
    return this->discovery;
}

/* Get the config file */
string arguments::get_config_file()
{
    return this->conf_filepath;
}

/* Get the delay value */
int arguments::get_delay()
{
    return this->delay;
}

/* Get the number of devices */
int arguments::get_number()
{
    return this->number;
}

/* This map converts only *SOME* of the SMA texts. Also, it will convert
   EXACTLY as it sees, and is case sensitive. This is deliberate */
void arguments::initialise_conversions()
{
    /* These are logged in every message. make them short */
    this->convert["Fehler"] = "error";
    this->convert["Netzueb."] = "checking grid";
    this->convert["Warten"] = "waiting";
    this->convert["Riso"] = "isol-resist";
    this->convert["U-Konst"] = "constant-volt";
    this->convert["Stoer."] = "failure";
    this->convert["-------"] = "ok";

    /* These are header entries. Make them long is OK */
    this->convert["Upv-Ist"] = "pv input voltage";
    this->convert["Upv-Soll"] = "internal pv voltage";
    this->convert["Fac"] = "grid freq";
    this->convert["Pac"] = "grid power";
    this->convert["Uac"] = "grid voltage";
    this->convert["Ipv"] = "pv panels current";
    this->convert["E-Total"] = "energy yield";
    this->convert["h-Total"] = "total operating hours";
    this->convert["h-On"] = "total start hours";
    this->convert["Netz-Ein"] = "total start-ups";
    this->convert["Iac-Ist"] = "current to grid";
    this->convert["Seriennummer"] = "serial number";
}

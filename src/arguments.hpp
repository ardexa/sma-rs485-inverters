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

#ifndef ARGUMENTS_HPP_INCLUDED
#define ARGUMENTS_HPP_INCLUDED

#include <string>
#include "arguments.hpp"
#include <iostream>
#include <map>
#include <unistd.h>

using namespace std;

#define DEFAULT_DEBUG_VALUE 0
#define DEFAULT_LOG_DIRECTORY "/opt/ardexa/sma/logs"
#define DEFAULT_CONF_DIRECTORY "/home/ardexa"
#define VERSION 1.12
#define DELAY 60

extern int g_debug;

/* This class is to call in the command line arguments */
class arguments
{
	public:
		/* methods are public */
		arguments();
		int initialize(int argc, char * argv[]);
		void usage();
		bool get_debug();
		bool get_discovery();
		string get_config_file();
		string get_log_directory();
		int get_delay();
		int get_number();
		void initialise_conversions();
		map <string, string> convert;

	private:
		/* members are private */
		bool debug;
		string conf_filepath;
		string log_directory;
		bool discovery; /* this is to simple list the available devices and exit */
		string usage_string;
		int delay;
		int number;
		
};	

#endif /* ARGUMENTS_HPP_INCLUDED */

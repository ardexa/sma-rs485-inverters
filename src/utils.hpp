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


#ifndef UTILS_HPP_INCLUDED
#define UTILS_HPP_INCLUDED

#include <string>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <algorithm>  

#define DATESIZE 64
#define DOUBLE_SIZE 64
#define PID_FILE "/run/ardexa-sma.pid"

extern int g_debug;

using namespace std;

int log_line(string directory, string filename, string line, string header, bool log_to_latest);
string get_current_date();
string get_current_datetime();
bool check_directory(string directory);
bool check_file(string file);
bool create_directory(string directory);
string convert_double(double number);
string replace_spaces(string incoming);
bool check_root();
bool check_pid_file();
void remove_pid_file();
string trim_whitespace(string raw_string);
bool convert_long(string incoming, long *outgoing);

#endif /* UTILS_HPP_INCLUDED */

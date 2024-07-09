#pragma once

#include <string>
#include <vector>
#include "CL.h"
#include "types.h"

using namespace std;

#define FUNC_NAME "smash"


/*A class to run MD5 in parallel, while
comparing to a certain value.*/
class smasher {
public:
	int smash(const uint block, char* cmpto);

	smasher();
	~smasher();

	bool get_ready() { return is_ready; }
private:
	// OpenCL data
	cl_platform_id platform;
	cl_device_id device;
	cl_context context;
	cl_command_queue command_queue;
	cl_int block_number;
	cl_mem output;
	cl_program program;
	cl_kernel kernel;

	string code;

	cl_int ret;
	
	bool is_ready;
	void set_ready() { if (ret != 0) is_ready = false; }

	/*Container specific functions*/

	void set_platform();

	void set_device();

	void create_context();

	void create_command_queue();

	void read_cl();

	void create_program();

	void create_kernel();

	void init();

	/*Operation specific functions*/

	void create_block_memory(char* out);

	void get_results(char* res);

	void set_args();

	void run();

	int find_match(const char* out, const char* cmpto);
};
#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include "smasher.h"
#include "log.h"

#define CL_FILE "smashMD5.cl"

void smasher::set_platform() {
	cl_uint ret_num_platforms;
	ret = clGetPlatformIDs(1, &platform, &ret_num_platforms);

	set_ready();

	// log data
	stringstream s;
	s << "Set platform to ID " << platform << ". NUMBER_OF_PLATFORMS = " << ret_num_platforms << ". Return code = " << getErrorString(ret);
	_log(s.str());
}

void smasher::set_device() {
	cl_uint ret_num_devices;
	ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &ret_num_devices);

	set_ready();

	// log data
	stringstream s;
	s << "Set device to ID " << device << ". NUMBER_OF_DEVICES = " << ret_num_devices << ". Return code = " << getErrorString(ret);
	_log(s.str());
}

void smasher::create_context() {
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);

	// log creation
	stringstream s;
	s << "Created context. Return code = " << getErrorString(ret);
	_log(s.str());

	set_ready();
}

void smasher::create_command_queue() {
	/*NOTE: Even though 'clCreateCommandQueue' is deprecated, 
	'clCreateCommandQueueWithProperties' causes error when not debugging!!!*/

	command_queue = clCreateCommandQueue(context, device, NULL, &ret);

	// log creation
	stringstream s;
	s << "Created command queue. Return code = " << getErrorString(ret);
	_log(s.str());

	set_ready();
}

void smasher::read_cl() {
	// open file
	fstream file(CL_FILE);

	// get file size
	int size = file.tellg();
	file.seekg(ios::beg);

	// read CL code
	stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	code = buffer.str();

	// log compilation
	stringstream s;
	s << "Read and compiled CL-code. Return code = " << getErrorString(ret);
	_log(s.str());

	set_ready();
}

void smasher::create_program() {
	// turn string into char buffer
	int count = code.size();
	char *buffer = &code[0];

	// compile code
	program = clCreateProgramWithSource(context, 1, (const char**)&buffer, (const size_t *)&count, &ret);
	ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

	/*
	//----------------------------------------------------------------------------------
	char a[4096];
	size_t length;
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 4096, &a, &length);
	cout.write(a, length);
	//----------------------------------------------------------------------------------
	*/

	// log creation
	stringstream s;
	s << "Created program. Return code = " << getErrorString(ret);
	_log(s.str());

	set_ready();
}

void smasher::create_kernel() {
	kernel = clCreateKernel(program, FUNC_NAME, &ret);

	// log creation
	stringstream s;
	s << "Created kernel. Return code = " << getErrorString(ret);
	_log(s.str());

	set_ready();
}

void smasher::init() {
	_log("Beginning initialization...");

	set_platform();
	set_device();
	create_context();
	create_command_queue();
	read_cl();
	create_program();
	create_kernel();

	_log("Initialization complete");
}

/*Operation specific functions*/

void smasher::create_block_memory(char* out) {
	stringstream s_log;

	output = clCreateBuffer(context, 
		CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, 
		TOTAL_MD5_SIZE,
		out,
		&ret);
	s_log << "Created output memory. count = " << TOTAL_MD5_SIZE << ". Return code = " << getErrorString(ret);

	_log(s_log.str());
}

void smasher::get_results(char* res) {
	// read result into out
	ret = clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, TOTAL_MD5_SIZE, res, 0, NULL, NULL);

	// log reading
	stringstream s;
	s << "Read output from GPU memory. Return code = " << getErrorString(ret);
	_log(s.str());
}

void smasher::set_args() {
	stringstream s;

	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output);
	s << "Set argument0 to output memory. Return code = " << getErrorString(ret);

	ret = clSetKernelArg(kernel, 1, sizeof(cl_int), &block_number);
	s << endl << "Set argument1 to block number " << block_number << ". Return code = " << getErrorString(ret);

	_log(s.str());
}

void smasher::run() {
	stringstream s1, s2;

	const size_t count = BLOCK_SIZE;

	_log("Running smasher...");

	// run sorting
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &count, NULL, NULL, NULL, NULL);
	clFinish(command_queue);
}

int smasher::smash(const uint block, char* cmpto) {
	uchar out[TOTAL_MD5_SIZE];

	// setup and run
	block_number = block;
	create_block_memory((char*)out);
	set_args();
	run();

	get_results((char*)out); // read results
	return find_match((char*)out, cmpto); // look for matching hash
}

int smasher::find_match(const char* out, const char* cmpto)  {
	// return index of key if exists
	for (uint k = 0; k < BLOCK_SIZE; ++k) {
		if (!memcmp(&out[k * KEY_SIZE], cmpto, KEY_SIZE)) // compare kth key in out to cmpto
			return k;
	}

	return -1; // no matching keys =(
}

smasher::smasher() {
	is_ready = true;
	init();
}
smasher::~smasher() {
	_log("Releasing OpenCL objects...");

	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(output);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	_log("Release complete");
}
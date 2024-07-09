#pragma once

using namespace std;


#define SMASHER_INIT_ERROR (1 << 0)

//#define LOG
#ifdef LOG
void _log(const string &msg);
#else
inline void _log(const string &msg) {}
#endif

const char *getErrorString(int error);

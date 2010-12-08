#include <stdio.h> // for printf(3)
#include <stdarg.h> // for va_start(3), va_end(3)

#include "us_helper.hh"

/*
 * This is an expanshion of the previous example showing how to use compiler builtins
 * and still enjoy the blessings of your own debugging facility.
 *
 * Notice:
 * - the use of pre-processor var args type of macro.
 * - the use of __attribute__((format)) for compile time type checking of arguments
 *   to the tracer.
 * - the use 
 *
 * 					Mark Veltzer
 *
 * TODO:
 */

// log levels, ideas copied shamelessly from syslog(3)
typedef enum _tracelevel {
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_NOTICE,
	LEVEL_WARNING,
	LEVEL_ERR,
	LEVEL_CRIT,
	LEVEL_ALERT,
	LEVEL_EMERG
} tracelevel;

inline void my_trace(tracelevel level,const char* file,const char* base_file, const int line, const char* func, const char* function, const char* pretty_function,const char* fmt,...) {
	// the current implemention is quite simplistic...
	printf("level is %d\n",level);
	printf("file is %s\n",file);
	printf("base_file is %s\n",base_file);
	printf("line is %d\n",line);
	printf("func is %s\n",func);
	printf("function %s\n",function);
	printf("pretty_function is %s\n",pretty_function);
	// now lets print the actual message...
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
inline void my_trace(tracelevel level,const char* file,const char* base_file,const int line, const char* func, const char* function, const char* pretty_function,const char* fmt,...) __attribute__((format(printf, 8, 9)));

// now wrap this up to get the right compiler macros...
// this is a pre-processor varargs type of tracer...
#define MY_TRACE(level,fmt,args...) my_trace(level,__FILE__,__BASE_FILE__,__LINE__,__func__,__FUNCTION__,__PRETTY_FUNCTION__,fmt, ## args)

class A {
	public:
		void thisMethod(int a, int b) {
			MY_TRACE(LEVEL_DEBUG,"did you know that %d+%d=%d?\n",a,b,a+b);
		}
};

int main(int argc, char **argv, char **envp) {
	A a;
	a.thisMethod(5,6);
	return 0;
}
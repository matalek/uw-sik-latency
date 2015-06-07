#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <exception>
#include <stdexcept>
#include <sstream>
#include "err.h"

void syserr(std::string e) {
	std::ostringstream ss;
	ss <<  "ERROR: " << e << " (" << errno << "; " << strerror(errno) << ")";
	throw std::runtime_error{ss.str()};
}

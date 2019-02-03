// mem_utils.hpp
// This file provides some utility APIs for dealing with the memory on the system.
// Currently, we only provide the total memory API, in the future, more will be added!
// Shamelessly copied from https://stackoverflow.com/questions/2513505/how-to-get-available-memory-c-g
#ifndef MEM_UTIL_HPP
#define MEM_UTIL_HPP

#if (__unix || __unix__ || __APPLE__ || __MACH__ || __linux__)
#include <unistd.h>

unsigned long long getTotalSystemMemory()
{
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  return pages * page_size;
}
#elif __WIN32 || __WIN64
#include <windows.h>

unsigned long long getTotalSystemMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#endif
#endif // MEM_UTIL_HPP
#pragma once

#define EN_LOG_TRACE
#ifdef EN_LOG_TRACE
#include <iostream>
#define LOG_TRACE(msg)             \
  do {                             \
    std::cerr << msg << std::endl; \
  } while (false)
#else
#define LOG_TRACE(msg)
#endif
#pragma once
#include <iostream>
#include <string>

#define LOG_INFO(msg)  std::cout << "[INFO]  " << msg << "\n"
#define LOG_WARN(msg)  std::cout << "[WARN]  " << msg << "\n"
#define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << "\n"

#pragma once
#include <cstdint>
#include <vector>
using uint = unsigned;
inline std::vector<unsigned long> delays;
inline void delay(unsigned long ms) { delays.push_back(ms); }

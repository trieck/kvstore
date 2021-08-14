#pragma once

#if !defined(_UNICODE) || !defined(UNICODE)
#error Unicode must be defined
#endif

#define NOMINMAX

#include <Windows.h>

#ifndef ASSERT
#include <crtdbg.h>
#define ASSERT _ASSERT
#endif // ASSERT

#include <boost/config/compiler/visualc.hpp>
#include <boost/format.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <locale>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


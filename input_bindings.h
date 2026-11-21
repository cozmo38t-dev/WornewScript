#pragma once
#include "vm.h"
#include <deque>

/* Input handling is moved out of the hot switch; this is critical: std::getline/stoll
 * inside the interpreter prevent gcc from optimizing the loop (20 -> 25 ms). */
Value read_value(int type_id, bool has_prompt, Value prompt,
                 std::deque<std::string>& heap);

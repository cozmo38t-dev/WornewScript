#pragma once
#include "vm.h"
#include <deque>
#include <string>

/* The implementation of the GUI builtins lives in a separate .cpp so as not to bloat
 * the interpreter's hot translation unit. */
Value gui_builtin(const std::string& name, const Value* args, int argc,
                  std::deque<std::string>& heap);

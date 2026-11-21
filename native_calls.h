#pragma once
#include "vm.h"

/* Calls to functions from C libraries in libs/ for the VM.
 * In generated C, the same functions are pasted in as text via $APT. */
Value call_native_builtin(const std::string& name, const Value* args, int argc);

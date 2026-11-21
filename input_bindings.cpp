#include "input_bindings.h"

#include <cctype>
#include <iostream>
#include <string>

Value read_value(int type_id, bool has_prompt, Value prompt,
                 std::deque<std::string>& heap) {
    if (has_prompt) std::cout << prompt.as_text() << std::flush;

    std::string line;
    std::getline(std::cin, line);

    if (type_id == 2) {           // str
        heap.push_back(line);
        return Value::text(heap.back().c_str());
    }
    if (type_id == 3) {           // bool
        std::string low;
        for (char c : line)
            low += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        bool b = !(low.empty() || low == "0" || low == "false" ||
                   low == "no" || low == "нет");
        return Value::number(b ? 1 : 0);
    }
    if (type_id == 0) {           // int
        try { return Value::number(static_cast<double>(std::stoll(line))); }
        catch (...) { return Value::number(0); }
    }
    try { return Value::number(std::stod(line)); }   // num
    catch (...) { return Value::number(0); }
}

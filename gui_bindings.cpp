#include "gui_bindings.h"

#define MINIGUI_IMPLEMENTATION
#include "runtime/ws_gui.h"

Value gui_builtin(const std::string& name, const Value* a, int argc,
                  std::deque<std::string>& heap) {
    auto N = [&](int i) -> double { return i < argc ? a[i].as_num() : 0.0; };
    auto S = [&](int i) -> const char* {
        return (i < argc && a[i].is_str()) ? a[i].str : "";
    };

    if (name == "window")    return Value::number(wsg_window(S(0), N(1), N(2)));
    if (name == "win_open")  return Value::number(wsg_open(N(0)));
    if (name == "win_begin") { wsg_begin(N(0)); return Value::number(0); }
    if (name == "win_end")   { wsg_end(N(0));   return Value::number(0); }
    if (name == "win_clear") { wsg_clear(N(0), N(1)); return Value::number(0); }
    if (name == "win_quit")  { wsg_quit(N(0));  return Value::number(0); }
    if (name == "win_width") return Value::number(wsg_width(N(0)));
    if (name == "win_height")return Value::number(wsg_height(N(0)));
    if (name == "win_fullscreen") { wsg_fullscreen(N(0), N(1)); return Value::number(0); }
    if (name == "win_is_fullscreen") return Value::number(wsg_is_fullscreen(N(0)));
    if (name == "rgb")       return Value::number(wsg_rgb(N(0), N(1), N(2)));
    if (name == "rect")      { wsg_rect(N(0), N(1), N(2), N(3), N(4), N(5)); return Value::number(0); }
    if (name == "panel")     { wsg_panel(N(0), N(1), N(2), N(3), N(4)); return Value::number(0); }
    if (name == "label")     { wsg_label(N(0), N(1), N(2), S(3)); return Value::number(0); }
    if (name == "label_ex")  { wsg_label_ex(N(0), N(1), N(2), S(3), N(4), N(5)); return Value::number(0); }
    if (name == "button")    return Value::number(wsg_button(N(0), N(1), N(2), N(3), N(4), S(5)));
    if (name == "checkbox")  return Value::number(wsg_checkbox(N(0), N(1), N(2), S(3), N(4)));
    if (name == "field") {
        heap.push_back(wsg_field(N(0), N(1), N(2), N(3), N(4), S(5)));
        return Value::text(heap.back().c_str());
    }
    if (name == "mouse_x")    return Value::number(wsg_mouse_x(N(0)));
    if (name == "mouse_y")    return Value::number(wsg_mouse_y(N(0)));
    if (name == "mouse_down") return Value::number(wsg_mouse_down(N(0)));
    if (name == "mouse_pressed") return Value::number(wsg_mouse_pressed(N(0)));
    if (name == "key_down")   return Value::number(wsg_key_down(N(0), N(1)));
    if (name == "key_pressed")return Value::number(wsg_key_pressed(N(0), N(1)));
    if (name == "font")       { wsg_font(N(0), N(1)); return Value::number(0); }
    if (name == "font_size")  return Value::number(wsg_font_size());
    if (name == "sleep")      { mg_sleep((int)N(0)); return Value::number(0); }
    return Value::number(0);
}

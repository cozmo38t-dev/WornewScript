CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -pedantic
WS_CPPFLAGS = -DWS_LIBS_DIR='"$(abspath libs)"'
LDLIBS   ?= -lX11
TARGET    = wornew

SRCS = main.cpp lexer.cpp parser.cpp compiler.cpp vm.cpp gui_bindings.cpp input_bindings.cpp native_compiler.cpp direct_compiler.cpp native_calls.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = common.h lexer.h parser.h compiler.h vm.h gui_bindings.h input_bindings.h native_compiler.h direct_compiler.h native_calls.h \
       $(wildcard runtime/*.h) $(wildcard libs/*.c)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp $(DEPS)
	$(CXX) $(WS_CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

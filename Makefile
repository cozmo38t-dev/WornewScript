CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -pedantic
WS_CPPFLAGS = -DWS_LIBS_DIR='"libs"'
LDLIBS   ?= -lX11
TARGET    = wornew

WIN_CXX    ?= x86_64-w64-mingw32-g++
WIN_TARGET  = wornew.exe

SRCS = main.cpp lexer.cpp parser.cpp compiler.cpp vm.cpp gui_bindings.cpp input_bindings.cpp native_compiler.cpp direct_compiler.cpp native_calls.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = common.h lexer.h parser.h compiler.h vm.h gui_bindings.h input_bindings.h native_compiler.h direct_compiler.h native_calls.h \
       $(wildcard runtime/*.h) $(wildcard libs/*.c)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

windows:
	$(WIN_CXX) $(CXXFLAGS) -DWS_LIBS_DIR='"libs"' -o $(WIN_TARGET) $(SRCS) \
		-static -static-libgcc -static-libstdc++ -lgdi32 -luser32

%.o: %.cpp $(DEPS)
	$(CXX) $(WS_CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(WIN_TARGET)

.PHONY: all clean windows

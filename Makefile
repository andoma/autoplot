O=build

PROG=autoplot

CXXFLAGS += -Iext/imgui -Iext/imgui/examples/ -std=c++14
LDFLAGS += -lm -lpthread

SRCS = src/main.cpp \
	ext/imgui/imgui.cpp \
	ext/imgui/imgui_draw.cpp \
	ext/imgui/imgui_widgets.cpp \
	ext/imgui/examples/imgui_impl_glfw.cpp \
	ext/imgui/examples/imgui_impl_opengl2.cpp \

include mkglue/glfw3.mk

include mkglue/prog.mk

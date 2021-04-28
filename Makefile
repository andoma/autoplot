O=build

PROG=autoplot


CXXFLAGS += -DIMGUI_USER_CONFIG=\"imcfg.h\"
CXXFLAGS += -I. -Iext/imgui -Iext/imgui/backends/ -Iext/implot -std=c++14
LDFLAGS += -lm -lpthread

SRCS = src/main.cpp \
	ext/imgui/imgui.cpp \
	ext/imgui/imgui_draw.cpp \
	ext/imgui/imgui_widgets.cpp \
	ext/imgui/imgui_tables.cpp \
	ext/imgui/backends/imgui_impl_glfw.cpp \
	ext/imgui/backends/imgui_impl_opengl2.cpp \

ALLDEPS += Makefile

include mkglue/glfw3.mk

include mkglue/prog.mk

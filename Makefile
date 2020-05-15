O=build

PROG=autoplot

CXXFLAGS += -Iext/imgui -Iext/imgui/examples/ -std=c++14

SRCS = src/main.cpp \
	ext/imgui/imgui.cpp \
	ext/imgui/imgui_draw.cpp \
	ext/imgui/imgui_widgets.cpp \
	ext/imgui/examples/imgui_impl_glfw.cpp \
	ext/imgui/examples/imgui_impl_opengl2.cpp \

LDFLAGS += -lglfw -framework OpenGL

OBJS := ${SRCS:%.cpp=${O}/%.o}

DEPS := ${OBJS:%.o=%.d} ${OBJS-prog:%.o=%.d}

ALLDEPS += Makefile

${PROG}: ${OBJS} ${ALLDEPS}
	@mkdir -p $(dir $@)
	${CXX} -o $@ ${OBJS} ${OBJS-prog} ${LDFLAGS}

${O}/%.o: %.cpp ${ALLDEPS} | $(SRCDEPS)
	@mkdir -p $(dir $@)
	${CXX} -MD -MP ${CPPFLAGS} ${CXXFLAGS} -o $@ -c $<


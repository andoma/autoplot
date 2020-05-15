#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>

#include <map>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

#define HISTORY_SIZE 256
#define HISTORY_MASK (HISTORY_SIZE - 1)

#define DERIVE_HISTORY_SIZE 16
#define DERIVE_HISTORY_MASK (DERIVE_HISTORY_SIZE - 1)


struct Chart {
  Chart()
    : min(0)
    , max(1)
    , kind(0)
    , gauge(0)
    , counter(0)
    , wrptr(0)
    , renormalize(0)
  {
    memset(history, 0, sizeof(history));
    for(int i = 0; i < DERIVE_HISTORY_MASK; i++) {
      derive_history[i] = INT64_MAX;
    }
  }
  float min;
  float max;

  int kind;
  float gauge;
  uint64_t counter;

  int wrptr;
  int renormalize;
  float history[HISTORY_SIZE];
  int64_t derive_history[DERIVE_HISTORY_SIZE];
};

static pthread_mutex_t chart_mutex = PTHREAD_MUTEX_INITIALIZER;

static std::map<std::string, Chart> charts;



static void
glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


static ImVec2 window_size = ImVec2(640, 1080);

void
resize_callback(GLFWwindow *window, int width, int height)
{
  window_size.x = width;
  window_size.y = height;
}


static void
parse_packet(const uint8_t *buf, int len)
{
  while(len > 2) {
    const uint8_t kind = buf[0];
    const uint8_t namelen = buf[1];

    buf += 2;
    len -= 2;

    if(len < namelen)
      return;


    std::string name((const char *)buf, namelen);
    buf += namelen;
    len -= namelen;

    auto &c = charts[name];

    c.kind = kind;
    switch(kind) {
    case 1: // Gauge
      if(len < sizeof(float))
        return;
      memcpy(&c.gauge, buf, sizeof(float));
      buf += sizeof(float);
      len -= sizeof(float);
      break;
    case 2: // Counter
      if(len < sizeof(uint64_t))
        return;
      memcpy(&c.counter, buf, sizeof(uint64_t));
      buf += sizeof(uint64_t);
      len -= sizeof(uint64_t);
      break;
    defaut:
      return;
    }
  }
}


static void *
inputthread(void *aux)
{
  uint8_t buf[256];

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd == -1) {
    perror("socket");
    exit(1);
  }

  int on = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(int)) < 0) {
    perror("setsockopt");
    exit(1);
  }

  struct sockaddr_in sin = {};
  sin.sin_family = AF_INET;
  sin.sin_port = htons(5678);

  if(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind");
    exit(1);
  }

  while(1) {

    int r = read(fd, buf, sizeof(buf) - 1);
    if(r < 0) {
      perror("read");
      exit(1);
    }

    pthread_mutex_lock(&chart_mutex);
    parse_packet(buf, r);
    pthread_mutex_unlock(&chart_mutex);
  }
}





static float
getvals(void *data, int index)
{
  Chart *c = (Chart *)data;
  int slot = (c->wrptr + 1 + index) & HISTORY_MASK;
  return c->history[slot];
}



int
main(int argc, char **argv)
{
  glfwSetErrorCallback(glfw_error_callback);
  if(!glfwInit()) {
    fprintf(stderr, "Unable to init GLFW\n");
    exit(1);
  }
  GLFWwindow *window = glfwCreateWindow(window_size.x, window_size.y,
                                        "Autoplot", NULL, NULL);
  if(window == NULL) {
    fprintf(stderr, "Unable to open window\n");
    exit(1);
  }

  glfwSetWindowPos(window, 50, 50);

  pthread_t tid;
  pthread_create(&tid, NULL, inputthread, NULL);

  glfwSetWindowSizeCallback(window, resize_callback);


  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL2_Init();

  ImVec4 clear_color = ImVec4(0.05,0.2,0.4,0);

  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(window_size);

    if(ImGui::Begin("Autoplot", NULL,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoBackground)) {

      for(auto &it : charts) {
        int64_t delta;
        auto &c = it.second;
        c.wrptr++;

        switch(c.kind) {
        case 1:
          break;

        case 2:

          delta = c.counter - c.derive_history[c.wrptr & DERIVE_HISTORY_MASK];
          c.derive_history[c.wrptr & DERIVE_HISTORY_MASK] = c.counter;
          c.gauge = delta < 0 ? 0 : delta * (60.0f / DERIVE_HISTORY_SIZE);
          break;
        }

        c.renormalize++;
        if(c.renormalize == 300) {
          c.renormalize = 0;
          float m = 0;
          for(int i = 0; i < HISTORY_SIZE; i++) {
            m = std::max(m, c.history[i]);
          }
          c.max = m;
        }

        c.history[c.wrptr & HISTORY_MASK] = c.gauge;
        c.max = std::max(c.max, c.gauge);

        char curval[64];
        snprintf(curval, sizeof(curval), "%f", c.gauge);

        char title[128];
        snprintf(title, sizeof(title), "%s (%.2f - %.2f)",
                 it.first.c_str(), c.min, c.max);

        ImGui::PlotLines(curval, getvals, (void *)&c, HISTORY_SIZE,
                         0, title, c.min, c.max, ImVec2(0, 100));
      }
    }
    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
  }
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}


#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <cstdlib>
#include <json.hpp>
#include <iostream>
#include <string>
#include <math.h>       
using json = nlohmann::json;

class Parcel {
public:
  int id, x1, y1, x2, y2, z1, z2;

  Parcel (json);

  int width();
  int height();
  int depth();
};

Parcel::Parcel (json j) {
  this->x1 = j["x1"];
  this->x2 = j["x2"];
  this->y1 = j["y1"];
  this->y2 = j["y2"];
  this->z1 = j["z1"];
  this->z2 = j["z2"];
};

int Parcel::width() {
  return this->x2 - this->x1;
}

int Parcel::height() {
  return this->y2 - this->y1;
}

int Parcel::depth() {
  return this->z2 - this->z1;
}

class Grid {
public:
  std::vector<Parcel*> parcels;
  std::string toString();
};

std::string Grid::toString () {
  return std::to_string(this->parcels.size());
}
Grid grid;

void parseTheParcels(emscripten_fetch_t *fetch) {
  // Null terminates data, dodgy, may overwrite something in memory
  ((char*)fetch->data)[fetch->numBytes]='\0';
    
  // printf ("%s\n", fetch->data);
  auto j = json::parse(fetch->data);
  auto p = j["parcels"];

  for (json::iterator it = p.begin(); it != p.end(); ++it) {
    // auto node = it.value()["address"];
    grid.parcels.push_back(new Parcel(it.value()));

    // std::cout << node << '\n';
  }

  std::cout << grid.toString() << "\n";
}

void downloadSucceeded(emscripten_fetch_t *fetch) {
  printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];

  parseTheParcels(fetch);  

  emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void downloadFailed(emscripten_fetch_t *fetch) {
  printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
  emscripten_fetch_close(fetch); // Also free data on failure.
}

void fetch() {
  printf("Fetching...");

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");

  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = downloadSucceeded;
  attr.onerror = downloadFailed;

  emscripten_fetch(&attr, "http://localhost:8000/test.json");
}

struct context
{
    SDL_Renderer *renderer;
    int iteration;
};

void mainloop(void *arg)
{
    context *ctx = static_cast<context*>(arg);
    SDL_Renderer *renderer = ctx->renderer;

    int offsetX = 225 + sin((float) ctx->iteration / 23.0) * 128.0;
    int offsetY = 225 + cos((float) ctx->iteration / 31.0) * 128.0;

    // example: draw a moving rectangle

    // red background
    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
    SDL_RenderClear(renderer);

    // map    

    SDL_SetRenderDrawColor(renderer, 255, 0, 192, 255 );
    for (Parcel *p : grid.parcels) {
      // std::cout << i << ' ';

      SDL_Rect r;
      r.x = p->x1 + offsetX;
      r.y = p->z1 + offsetY;
      r.w = p->width();
      r.h = p->depth();
      SDL_RenderFillRect(renderer, &r);
      SDL_RenderDrawRect(renderer, &r);
    }

    // moving blue rectangle
    // SDL_Rect r;
    // r.x = ctx->iteration % 255;
    // r.y = 50;
    // r.w = 50;
    // r.h = 50;
    // SDL_SetRenderDrawColor(renderer, 255, 0, 192, 255 );
    // SDL_RenderFillRect(renderer, &r );

    SDL_RenderPresent(renderer);

    ctx->iteration++;
}

int main()
{
  SDL_version compiled;
  SDL_version linked;

  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  printf("We compiled against SDL version %d.%d.%d ...\n",
         compiled.major, compiled.minor, compiled.patch);
  printf("But we are linking against SDL version %d.%d.%d.\n",
         linked.major, linked.minor, linked.patch);

  SDL_Init(SDL_INIT_VIDEO);
  // SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);

  context ctx;
  ctx.renderer = renderer;
  ctx.iteration = 0;

  fetch();

  const int simulate_infinite_loop = 1; // call the function repeatedly
  const int fps = -1; // call the function as fast as the browser wants to render (typically 60fps)
  emscripten_set_main_loop_arg(mainloop, &ctx, fps, simulate_infinite_loop);
  
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}

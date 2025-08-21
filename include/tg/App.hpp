#ifndef TG_APP_HPP
#define TG_APP_HPP

#include "tg/Renderer.hpp"
#include "tg/generator.hpp"

#include <SDL3/SDL.h>

namespace tg {

class App {
public:
    App(int argc, char* argv[]);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;
    
    void run();

private:
    SDL_Window* _window;
    Renderer _renderer;

};

} // namespace tg

#endif // TG_APP_HPP
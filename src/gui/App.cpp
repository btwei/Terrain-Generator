#include "tg/App.hpp"

namespace tg
{

App::App(int, char*[]) { }

App::~App() { }

void App::run() {
    // Initialize SDL and create a window
    SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	
	_window = SDL_CreateWindow(
		"Terrain Generator",
		800, 600,
		window_flags
	);

    // Initialize the renderer
    _renderer.init(_window);

    // Main application loop
    while (_renderer.isRunning()) {
        SDL_Event e;
        while(SDL_PollEvent(&e) != false) {
            _renderer.handleEvent(e);
        }

        _renderer.update();
        _renderer.render();
    }

    // Cleanup resources
    _renderer.cleanup();

    SDL_DestroyWindow(_window);
}

} // namespace tg

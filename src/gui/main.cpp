#include <iostream>
#include <stdexcept>

#include "tg/App.hpp"

int main(int argc, char* argv[]) {

    try {
        tg::App app(argc, argv);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#include <iostream>
#include "Arena.h"

int main(int argc, char* argv[]) {
    // Check if the user provided a config file
    if (argc < 2) {
        std::cerr << "Usage: ./RobotWarz <config_file.txt>\n";
        return 1;
    }

    std::string config_file = argv[1];
    Arena arena;

    // 1. Load the configuration
    if (!arena.load_config(config_file)) {
        std::cerr << "Failed to load configuration. Exiting.\n";
        return 1;
    }

    // 2. Load the robots from the "robots" directory
    // Ensure you have a folder named "robots" in the same directory as the executable!
    if (!arena.load_robots("robots")) {
        std::cerr << "No robots loaded. Exiting.\n";
        return 1;
    }

    // 3. Start the carnage
    arena.run();

    return 0;
}
#pragma once

#include <vector>
#include <string>
#include <memory>
#include "RobotBase.h"

class Arena {
private:
    // --- Configuration Parameters ---
    int m_height;
    int m_width;
    int m_max_rounds;
    double m_sleep_interval;
    bool m_game_state_live;
    int m_num_flamethrowers;
    int m_num_pits;
    int m_num_mounds;

    // --- Arena State ---
    // Using a 2D vector of chars for easy resizing based on config
    std::vector<std::vector<char>> m_board;
    
    // We store the robots as raw pointers because they are dynamically 
    // loaded from shared libraries. We will manage their memory carefully!
    std::vector<RobotBase*> m_robots;
    
    // To handle the dlopen handles so we can clean them up at the end
    std::vector<void*> m_lib_handles;

    // --- Private Helper Methods ---
    void place_obstacles();
    void place_robots();
    void update_board(); // Refreshes the 2D char array based on robot positions
    void print_board(int round_number);
    
    // Core Turn Logic
    std::vector<RadarObj> do_radar_scan(RobotBase* robot, int direction);
    void handle_shot(RobotBase* shooter, int target_row, int target_col);
    void handle_move(RobotBase* robot, int direction, int speed);

public:
    Arena();
    ~Arena();

    // Setup methods
    bool load_config(const std::string& config_filename);
    bool load_robots(const std::string& robots_directory);

    // Main game loop
    void run();
};
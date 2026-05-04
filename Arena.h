#pragma once

#include <vector>
#include <string>
#include <memory>
#include "RobotBase.h"

class Arena {
private:
    int m_height;
    int m_width;
    int m_max_rounds;
    double m_sleep_interval;
    bool m_game_state_live;
    int m_num_flamethrowers;
    int m_num_pits;
    int m_num_mounds;

    std::vector<std::vector<char>> m_board;
    
    std::vector<RobotBase*> m_robots;
    
    std::vector<void*> m_lib_handles;

    void place_obstacles();
    void place_robots();
    void update_board(); 
    void print_board(int round_number);
    
    std::vector<RadarObj> do_radar_scan(RobotBase* robot, int direction);
    void handle_shot(RobotBase* shooter, int target_row, int target_col);
    void handle_move(RobotBase* robot, int direction, int speed);

public:
    Arena();
    ~Arena();

    bool load_config(const std::string& config_filename);
    bool load_robots(const std::string& robots_directory);

    void run();
};
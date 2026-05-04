#include "RobotBase.h"
#include <cstdlib>
#include <cmath>
#include <algorithm> 

class Robot_Berserker : public RobotBase {
private:
    int m_target_row;
    int m_target_col;
    bool m_has_target;
    int m_current_radar_dir;
    int m_pursuit_dir;

public:
    Robot_Berserker() : RobotBase(6, 1, hammer) {
        m_name = "SmartBerserker";
        m_has_target = false;
        m_current_radar_dir = 1;
        m_pursuit_dir = 1;
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = m_current_radar_dir;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                m_pursuit_dir = m_current_radar_dir;
                break;
            }
        }

        m_current_radar_dir++;
        if (m_current_radar_dir > 8) {
            m_current_radar_dir = 1;
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (m_has_target) {
            int my_r, my_c;
            get_current_location(my_r, my_c);
            
            if (std::abs(my_r - m_target_row) <= 1 && std::abs(my_c - m_target_col) <= 1) {
                shot_row = m_target_row;
                shot_col = m_target_col;
                m_has_target = false; 
                return true;
            }
        }
        return false; 
    }

    void get_move_direction(int& direction, int& distance) override {
        if (m_has_target) {
            int my_r, my_c;
            get_current_location(my_r, my_c);
            
            int dist_r = std::abs(my_r - m_target_row);
            int dist_c = std::abs(my_c - m_target_col);
            int max_dist_to_target = std::max(dist_r, dist_c);

            direction = m_pursuit_dir;
            
            if (max_dist_to_target > 1 && max_dist_to_target <= get_move_speed() + 1) {
                distance = max_dist_to_target - 1; 
            } else {
                distance = get_move_speed(); 
            }

        } else {
            direction = m_current_radar_dir;
            distance = 1; 
        }
    }
};

extern "C" RobotBase* create_robot() { return new Robot_Berserker(); }
extern "C" const char* robot_summary() { return "Hunts targets systematically and paces its movement."; }
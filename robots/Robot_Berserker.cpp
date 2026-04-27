#include "RobotBase.h"
#include <cstdlib>
#include <cmath>
#include <algorithm> // For std::max

class Robot_Berserker : public RobotBase {
private:
    int m_target_row;
    int m_target_col;
    bool m_has_target;
    int m_current_radar_dir;
    int m_pursuit_dir;

public:
    // Trade-off: 6 Move, 1 Armor (Total 7). Weapon: Hammer.
    Robot_Berserker() : RobotBase(6, 1, hammer) {
        m_name = "SmartBerserker";
        m_has_target = false;
        m_current_radar_dir = 1;
        m_pursuit_dir = 1;
    }

    void get_radar_direction(int& radar_direction) override {
        // UPGRADE 1: Methodical sweep instead of random spinning
        radar_direction = m_current_radar_dir;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                m_pursuit_dir = m_current_radar_dir; // Lock onto this direction!
                break;
            }
        }

        // Move the radar to the next sector for the next turn
        m_current_radar_dir++;
        if (m_current_radar_dir > 8) {
            m_current_radar_dir = 1;
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (m_has_target) {
            int my_r, my_c;
            get_current_location(my_r, my_c);
            
            // If they are exactly 1 space away or less, SMASH!
            if (std::abs(my_r - m_target_row) <= 1 && std::abs(my_c - m_target_col) <= 1) {
                shot_row = m_target_row;
                shot_col = m_target_col;
                m_has_target = false; // Target engaged, reset to hunting mode next turn
                return true;
            }
        }
        return false; 
    }

    void get_move_direction(int& direction, int& distance) override {
        if (m_has_target) {
            int my_r, my_c;
            get_current_location(my_r, my_c);
            
            // Calculate exactly how far away they are
            int dist_r = std::abs(my_r - m_target_row);
            int dist_c = std::abs(my_c - m_target_col);
            int max_dist_to_target = std::max(dist_r, dist_c);

            direction = m_pursuit_dir;
            
            // UPGRADE 2: Don't overshoot!
            // Move exactly enough to land adjacent to the target.
            if (max_dist_to_target > 1 && max_dist_to_target <= get_move_speed() + 1) {
                distance = max_dist_to_target - 1; 
            } else {
                distance = get_move_speed(); // Otherwise, run as fast as possible to catch up
            }

        } else {
            // UPGRADE 3: No target? Move cautiously (1 space) while sweeping radar
            direction = m_current_radar_dir;
            distance = 1; 
        }
    }
};

extern "C" RobotBase* create_robot() { return new Robot_Berserker(); }
extern "C" const char* robot_summary() { return "Hunts targets systematically and paces its movement."; }
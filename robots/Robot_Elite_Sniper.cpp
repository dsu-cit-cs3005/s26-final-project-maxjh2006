#include "RobotBase.h"
#include <cstdlib>
#include <ctime>

class Robot_Sniper : public RobotBase {
private:
    int m_current_radar_dir;
    int m_target_row;
    int m_target_col;
    bool m_has_target;

public:
    // Trade-off: 4 Move, 3 Armor (Total 7). Weapon: Railgun.
    Robot_Sniper() : RobotBase(4, 3, railgun) {
        m_name = "SniperBot";
        m_current_radar_dir = 1;
        m_target_row = -1;
        m_target_col = -1;
        m_has_target = false;
        std::srand(std::time(nullptr)); // Seed random for movement
    }

    void get_radar_direction(int& radar_direction) override {
        // Sweep the radar in a circle (directions 1 through 8)
        radar_direction = m_current_radar_dir;
        m_current_radar_dir++;
        if (m_current_radar_dir > 8) {
            m_current_radar_dir = 1;
        }
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        m_has_target = false; // Reset target each scan

        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') { // Found a live robot!
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                break; // Lock on to the first target we see
            }
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (m_has_target) {
            shot_row = m_target_row;
            shot_col = m_target_col;
            m_has_target = false; // Reset after taking the shot
            return true;          // Tell the Arena we are shooting!
        }
        return false; // Not shooting this turn
    }

    void get_move_direction(int& direction, int& distance) override {
        // If we didn't shoot, we will move to avoid taking hits.
        // Pick a random direction (1-8) and move our max speed.
        direction = (std::rand() % 8) + 1; 
        distance = get_move_speed(); 
    }
};

// =====================================================================
// Required C-style exports for the Arena to dynamically load this robot
// =====================================================================

extern "C" RobotBase* create_robot() {
    return new Robot_Sniper();
}

extern "C" const char* robot_summary() {
    return "Sweeps radar, railguns, moves randomly.";
}
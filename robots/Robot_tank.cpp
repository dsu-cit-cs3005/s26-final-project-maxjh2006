#include "RobotBase.h"
#include <cstdlib>

class Robot_Tank : public RobotBase {
private:
    int m_current_radar_dir;
    int m_target_row;
    int m_target_col;
    bool m_has_target;

public:
    Robot_Tank() : RobotBase(1, 6, flamethrower) {
        m_name = "HeavyTank";
        m_current_radar_dir = 1;
        m_has_target = false;
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = m_current_radar_dir;
        m_current_radar_dir = (m_current_radar_dir % 8) + 1;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        m_has_target = false;
        for (const auto& obj : radar_results) {
            if (obj.m_type == 'R') {
                m_target_row = obj.m_row;
                m_target_col = obj.m_col;
                m_has_target = true;
                break;
            }
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (m_has_target) {
            shot_row = m_target_row;
            shot_col = m_target_col;
            m_has_target = false; 
            return true; 
        }
        return false; 
    }

    void get_move_direction(int& direction, int& distance) override {
        direction = (std::rand() % 8) + 1; 
        distance = get_move_speed(); 
    }
};

extern "C" RobotBase* create_robot() { return new Robot_Tank(); }
extern "C" const char* robot_summary() { return "Slow, armored flamethrower."; }

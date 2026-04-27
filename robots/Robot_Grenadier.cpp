#include "RobotBase.h"
#include <cstdlib>

class Robot_Grenadier : public RobotBase {
private:
    int m_current_radar_dir;
    int m_target_row;
    int m_target_col;
    bool m_has_target;

public:
    // Trade-off: 3 Move, 4 Armor (Total 7). Weapon: Grenade.
    Robot_Grenadier() : RobotBase(3, 4, grenade) {
        m_name = "Demolitionist";
        m_current_radar_dir = 1;
        m_has_target = false;
    }

    void get_radar_direction(int& radar_direction) override {
        // Methodical 360-degree sweep
        radar_direction = m_current_radar_dir;
        m_current_radar_dir++;
        if (m_current_radar_dir > 8) {
            m_current_radar_dir = 1;
        }
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
        // Only take the shot if we see a target AND we have ammo left
        if (m_has_target && get_grenades() > 0) {
            shot_row = m_target_row;
            shot_col = m_target_col;
            m_has_target = false; 
            return true; // Fire in the hole!
        }
        return false; 
    }

    void get_move_direction(int& direction, int& distance) override {
        if (get_grenades() <= 0) {
            // PANIC MODE: Out of ammo! Run around wildly at max speed!
            direction = (std::rand() % 8) + 1;
            distance = get_move_speed();
        } else {
            // TACTICAL MODE: Creep around slowly to avoid pits and flamethrowers
            direction = (std::rand() % 8) + 1;
            distance = 1; 
        }
    }
};

extern "C" RobotBase* create_robot() { return new Robot_Grenadier(); }
extern "C" const char* robot_summary() { return "Lobs AoE explosives, panics when out of ammo."; }
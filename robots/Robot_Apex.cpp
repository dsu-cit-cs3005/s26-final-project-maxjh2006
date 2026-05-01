#include "RobotBase.h"
#include <cstdlib>
#include <cmath>

class Robot_Apex : public RobotBase {
private:
    int m_current_radar_dir;
    int m_target_row;
    int m_target_col;
    bool m_has_target;

    // Helper to translate coordinate directions back into Arena 1-8 directions
    int get_dir_from_deltas(int dr, int dc) {
        if (dr == -1 && dc == 0) return 1;  // North
        if (dr == -1 && dc == 1) return 2;  // Northeast
        if (dr == 0 && dc == 1) return 3;   // East
        if (dr == 1 && dc == 1) return 4;   // Southeast
        if (dr == 1 && dc == 0) return 5;   // South
        if (dr == 1 && dc == -1) return 6;  // Southwest
        if (dr == 0 && dc == -1) return 7;  // West
        if (dr == -1 && dc == -1) return 8; // Northwest
        return 1; // Default
    }

public:
    // Trade-off: 4 Move (high mobility to kite), 3 Armor (survivability). Weapon: Railgun.
    Robot_Apex() : RobotBase(4, 3, railgun) {
        m_name = "ApexPredator";
        m_current_radar_dir = 1;
        m_has_target = false;
    }

    void get_radar_direction(int& radar_direction) override {
        // Continuous 360-degree tactical sweep
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
                break; // Lock onto the first target spotted
            }
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (!m_has_target) return false;

        int my_r, my_c;
        get_current_location(my_r, my_c);
        
        int dr = std::abs(m_target_row - my_r);
        int dc = std::abs(m_target_col - my_c);

        // TOURNAMENT WINNING LOGIC: Only fire if perfectly aligned!
        // (Same row, same column, or perfect 45-degree diagonal)
        if (dr == 0 || dc == 0 || dr == dc) {
            shot_row = m_target_row;
            shot_col = m_target_col;
            m_has_target = false; // Reset after firing
            return true; 
        }
        
        // If not aligned, don't waste the turn. Save it for movement!
        return false; 
    }

    void get_move_direction(int& direction, int& distance) override {
        int my_r, my_c;
        get_current_location(my_r, my_c);

        if (m_has_target) {
            int diff_r = m_target_row - my_r;
            int diff_c = m_target_col - my_c;
            
            // KITING LOGIC: If they are too close (Flamethrower/Hammer range), run away!
            if (std::abs(diff_r) <= 4 && std::abs(diff_c) <= 4) {
                int run_dr = (diff_r > 0) ? -1 : (diff_r < 0 ? 1 : 0);
                int run_dc = (diff_c > 0) ? -1 : (diff_c < 0 ? 1 : 0);
                // If they are on exactly the same row/col, shift diagonally to escape
                if (run_dr == 0) run_dr = 1;
                if (run_dc == 0) run_dc = 1;
                
                direction = get_dir_from_deltas(run_dr, run_dc);
                distance = get_move_speed(); // Sprint!
                return;
            }

            // ALIGNMENT LOGIC: If we are safe, step into alignment for a railgun shot
            int move_dr = 0, move_dc = 0;
            if (std::abs(diff_r) < std::abs(diff_c)) {
                move_dr = (diff_r > 0) ? 1 : -1; // Step vertically to match row
            } else {
                move_dc = (diff_c > 0) ? 1 : -1; // Step horizontally to match col
            }

            direction = get_dir_from_deltas(move_dr, move_dc);
            distance = 1; // Just step exactly as much as needed to align

        } else {
            // No target seen? Move cautiously while sweeping
            direction = (std::rand() % 8) + 1;
            distance = 1;
        }
    }
};

extern "C" RobotBase* create_robot() { return new Robot_Apex(); }
extern "C" const char* robot_summary() { return "Tournament Build: Kites enemies and calculates perfect railgun geometry."; }
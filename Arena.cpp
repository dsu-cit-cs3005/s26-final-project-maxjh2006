#include "Arena.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <dlfcn.h>
#include <random>
#include <iomanip>
#include <thread>
#include <chrono>
#include <cmath>

// Constructor: Set some safe default values just in case the config file is missing something
Arena::Arena() : 
    m_height(20), m_width(20), m_max_rounds(1000), 
    m_sleep_interval(0.5), m_game_state_live(true), 
    m_num_flamethrowers(5), m_num_pits(5), m_num_mounds(5) 
{}


bool Arena::load_config(const std::string& config_filename) {
    std::ifstream file(config_filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << config_filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string key;

        // Parse up to the colon ':'
        if (std::getline(iss, key, ':')) {
            std::string value;
            // Get the rest of the line after the colon
            std::getline(iss, value); 

            // Route the value to the correct class member
            if (key == "Arena_Size") {
                std::istringstream val_stream(value);
                val_stream >> m_height >> m_width;
            } else if (key == "Max_Rounds") {
                m_max_rounds = std::stoi(value);
            } else if (key == "Sleep_interval") {
                m_sleep_interval = std::stod(value);
            } else if (key == "Game_State_Live") {
                // Check if the string contains "true"
                m_game_state_live = (value.find("true") != std::string::npos);
            } else if (key == "Flamethrowers") {
                m_num_flamethrowers = std::stoi(value);
            } else if (key == "Pits") {
                m_num_pits = std::stoi(value);
            } else if (key == "Mounds") {
                m_num_mounds = std::stoi(value);
            }
        }
    }

    file.close();

    m_board.assign(m_height, std::vector<char>(m_width, '.'));
    
    std::cout << "Successfully loaded configuration. Board sized to " << m_height << "x" << m_width << ".\n";
    return true;
}


Arena::~Arena() {
    for (RobotBase* robot : m_robots) {
        delete robot;
    }
    m_robots.clear();

    for (void* handle : m_lib_handles) {
        if (handle) {
            dlclose(handle);
        }
    }
    m_lib_handles.clear();
}

bool Arena::load_robots(const std::string& robots_directory) {
    std::string markers = "@#$%&!*^~?";
    int marker_idx = 0;

    if (!std::filesystem::exists(robots_directory) || !std::filesystem::is_directory(robots_directory)) {
        std::cerr << "Error: Directory '" << robots_directory << "' not found.\n";
        return false;
    }

    std::vector<std::string> loaded_libs; // Keep track of what we've loaded

    for (const auto& entry : std::filesystem::directory_iterator(robots_directory)) {
        if (!entry.is_regular_file()) continue;

        std::string filepath = entry.path().string();
        std::string filename = entry.path().filename().string();

        if (filename.find("Robot_") != 0) continue; // Must start with Robot_

        std::string shared_lib = "";

        if (filename.find(".cpp") != std::string::npos) {
            // Scenario A: It's a source code file. Compile it!
            shared_lib = filepath.substr(0, filepath.find_last_of('.')) + ".so";
            
            // Check if we already loaded the pre-compiled version of this
            if (std::find(loaded_libs.begin(), loaded_libs.end(), shared_lib) != loaded_libs.end()) {
                continue; 
            }

            std::string compile_cmd = "g++ -shared -fPIC -o " + shared_lib + " " + filepath + " RobotBase.o -I. -std=c++20";
            std::cout << "Compiling " << filename << " to " << shared_lib << "...\n";
            
            int compile_result = std::system(compile_cmd.c_str());
            if (compile_result != 0) {
                std::cerr << "Failed to compile " << filename << ".\n";
                continue; 
            }
        } 
        else if (filename.find(".so") != std::string::npos) {
            shared_lib = filepath;
            
            if (std::find(loaded_libs.begin(), loaded_libs.end(), shared_lib) != loaded_libs.end()) {
                continue; 
            }
        } 
        else {
            continue; 
        }

        void* handle = dlopen(shared_lib.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Failed to load " << shared_lib << ": " << dlerror() << std::endl;
            continue;
        }

        RobotFactory create_robot = (RobotFactory)dlsym(handle, "create_robot");
        if (!create_robot) {
            std::cerr << "Failed to find create_robot in " << shared_lib << ": " << dlerror() << std::endl;
            dlclose(handle);
            continue;
        }

        RobotBase* robot = create_robot();
        if (robot) {
            robot->m_character = markers[marker_idx % markers.length()];
            marker_idx++;
            robot->set_boundaries(m_height, m_width);

            m_robots.push_back(robot);
            m_lib_handles.push_back(handle);
            loaded_libs.push_back(shared_lib); 
            
            std::cout << "Successfully loaded robot: " << robot->m_name 
                      << " (" << robot->m_character << ")\n";
        }
    }

    // Return true if we loaded at least one robot
    return !m_robots.empty();
}

void Arena::place_obstacles() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> row_dist(0, m_height - 1);
    std::uniform_int_distribution<> col_dist(0, m_width - 1);

    auto place_item = [&](int count, char symbol) {
        int placed = 0;
        int max_attempts = m_height * m_width * 10; 
        
        while (placed < count && max_attempts > 0) {
            int r = row_dist(gen);
            int c = col_dist(gen);
            
            if (m_board[r][c] == '.') {
                m_board[r][c] = symbol;
                placed++;
            }
            max_attempts--;
        }
    };

    place_item(m_num_mounds, 'M');
    place_item(m_num_pits, 'P');
    place_item(m_num_flamethrowers, 'F');
}

void Arena::place_robots() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> row_dist(0, m_height - 1);
    std::uniform_int_distribution<> col_dist(0, m_width - 1);

    for (RobotBase* robot : m_robots) {
        int max_attempts = m_height * m_width * 10;
        
        while (max_attempts > 0) {
            int r = row_dist(gen);
            int c = col_dist(gen);
            
            if (m_board[r][c] == '.') {
                bool occupied = false;
                
                for (RobotBase* other : m_robots) {
                    int orow = -1, ocol = -1;
                    other->get_current_location(orow, ocol);
                    if (orow == r && ocol == c) {
                        occupied = true;
                        break;
                    }
                }
                
                if (!occupied) {
                    robot->move_to(r, c); 
                    break;
                }
            }
            max_attempts--;
        }
    }
}

void Arena::update_board() {
}

void Arena::print_board(int round_number) {
    std::cout << "\033[2J\033[1;1H";

    std::cout << "\n        =========== starting round " << round_number << " ===========\n\n";

    std::cout << "  --- COMPETITOR STATUS --------------------------------------\n";
    
    for (RobotBase* robot : m_robots) {
        if (robot->get_health() > 0) {
            std::cout << "  [ALIVE] " << robot->m_name 
                      << " | Health: " << robot->get_health() 
                      << " | Speed: " << robot->get_move_speed() << "\n";
        } else {
            std::cout << "  \033[31m[DEAD]  " << robot->m_name << "\033[0m\n";
        }
    }
    std::cout << "  ------------------------------------------------------------\n\n";

    std::cout << "    ";
    for (int c = 0; c < m_width; ++c) {
        std::cout << std::setw(2) << c << " ";
    }
    std::cout << "\n";

    for (int r = 0; r < m_height; ++r) {
        std::cout << std::setw(2) << r << " ";
        
        for (int c = 0; c < m_width; ++c) {
            std::string cell_display = " . ";
            char base_item = m_board[r][c]; 
            bool robot_here = false;

            for (RobotBase* robot : m_robots) {
                int rob_r, rob_c;
                robot->get_current_location(rob_r, rob_c);
                
                if (rob_r == r && rob_c == c) {
                    if (robot->get_health() <= 0) {
                        cell_display = " X"; // Dead robot
                        cell_display += robot->m_character;
                    } else {
                        cell_display = " R"; // Live robot
                        cell_display += robot->m_character;
                    }
                    robot_here = true;
                    break; 
                }
            }

            // If no robot is here, print the terrain
            if (!robot_here) {
                if (base_item != '.') {
                    cell_display = "  ";
                    cell_display += base_item;
                }
            }

            std::cout << cell_display;
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void Arena::run() {
    place_obstacles();
    place_robots();

    for (int round = 1; round <= m_max_rounds; ++round) {
        // 1. Check for a winner before the round starts
        int alive_count = 0;
        RobotBase* winner = nullptr;
        for (RobotBase* robot : m_robots) {
            if (robot->get_health() > 0) {
                alive_count++;
                winner = robot;
            }
        }

        if (alive_count <= 1) {
            print_board(round);
            if (winner) {
                std::cout << "\n*** GAME OVER! The winner is " << winner->m_name 
                          << " (" << winner->m_character << ")! ***\n";
            } else {
                std::cout << "\n*** GAME OVER! It's a draw! No robots survived. ***\n";
            }
            break;
        }

        print_board(round);

        // 2. Give each robot a turn
        for (RobotBase* robot : m_robots) {
            if (robot->get_health() <= 0) continue; // Skip dead robots

            // --- Radar Phase ---
            int radar_dir = 0;
            robot->get_radar_direction(radar_dir);
            std::vector<RadarObj> radar_results = do_radar_scan(robot, radar_dir);
            robot->process_radar_results(radar_results);

            // --- Action Phase ---
            int target_row = -1, target_col = -1;
            bool wants_to_shoot = robot->get_shot_location(target_row, target_col);

            if (wants_to_shoot) {
                handle_shot(robot, target_row, target_col);
            } else {
                int move_dir = 0, move_dist = 0;
                robot->get_move_direction(move_dir, move_dist);
                if (move_dist > 0 && move_dir > 0 && move_dir <= 8) {
                    handle_move(robot, move_dir, move_dist);
                }
            }
        }

        // 3. Sleep if watching live
        if (m_game_state_live) {
            std::this_thread::sleep_for(std::chrono::duration<double>(m_sleep_interval));
        }
    }
}

std::vector<RadarObj> Arena::do_radar_scan(RobotBase* robot, int direction) {
    std::vector<RadarObj> results;
    int r, c;
    robot->get_current_location(r, c);

    // Direction 0: Scan the immediate 8 surrounding cells
    if (direction == 0) {
        for (int i = 1; i <= 8; ++i) {
            int nr = r + directions[i].first;
            int nc = c + directions[i].second;
            
            if (nr >= 0 && nr < m_height && nc >= 0 && nc < m_width) {
                // If it's not empty, we log what is there
                if (m_board[nr][nc] != '.') {
                    results.push_back(RadarObj(m_board[nr][nc], nr, nc));
                }
                // Check if another robot is there
                for (RobotBase* other : m_robots) {
                    if (other == robot) continue;
                    int orow, ocol;
                    other->get_current_location(orow, ocol);
                    if (orow == nr && ocol == nc) {
                        char type = (other->get_health() > 0) ? 'R' : 'X';
                        results.push_back(RadarObj(type, nr, nc));
                    }
                }
            }
        }
        return results;
    }

    // Directions 1-8: Cast a 3-wide ray to the edge of the board
    int dr = directions[direction].first;
    int dc = directions[direction].second;

    // Define perpendicular offsets to make the beam 3-wide
    // If moving vertically/horizontally, we check the sides.
    // If moving diagonally, we check the adjacent orthogonal cells.
    int p1_dr = 0, p1_dc = 0, p2_dr = 0, p2_dc = 0;
    if (dr == 0) { p1_dr = -1; p2_dr = 1; } // Moving left/right, scan above and below
    else if (dc == 0) { p1_dc = -1; p2_dc = 1; } // Moving up/down, scan left and right
    else { 
        // Diagonal: orthogonal offsets
        p1_dr = dr; p1_dc = 0; 
        p2_dr = 0; p2_dc = dc; 
    }

    int curr_r = r + dr;
    int curr_c = c + dc;

    // Keep stepping forward until we hit the edge of the board
    while (curr_r >= 0 && curr_r < m_height && curr_c >= 0 && curr_c < m_width) {
        
        // The three cells in this "slice" of the beam
        std::pair<int, int> scan_cells[3] = {
            {curr_r, curr_c},                       // Center of beam
            {curr_r + p1_dr, curr_c + p1_dc},       // Left/Top side
            {curr_r + p2_dr, curr_c + p2_dc}        // Right/Bottom side
        };

        for (int i = 0; i < 3; ++i) {
            int scan_r = scan_cells[i].first;
            int scan_c = scan_cells[i].second;

            // Ensure this specific slice cell is within bounds
            if (scan_r >= 0 && scan_r < m_height && scan_c >= 0 && scan_c < m_width) {
                // Check terrain
                if (m_board[scan_r][scan_c] != '.') {
                    results.push_back(RadarObj(m_board[scan_r][scan_c], scan_r, scan_c));
                }
                // Check for robots
                for (RobotBase* other : m_robots) {
                    if (other == robot) continue;
                    int orow, ocol;
                    other->get_current_location(orow, ocol);
                    if (orow == scan_r && ocol == scan_c) {
                        char type = (other->get_health() > 0) ? 'R' : 'X';
                        results.push_back(RadarObj(type, scan_r, scan_c));
                    }
                }
            }
        }
        curr_r += dr;
        curr_c += dc;
    }

    return results;
}

void Arena::handle_shot(RobotBase* shooter, int target_row, int target_col) {
    int sr, sc;
    shooter->get_current_location(sr, sc);
    WeaponType weapon = shooter->get_weapon();

    // Determine iterative direction (delta_x and delta_y)
    int dr = (target_row > sr) ? 1 : ((target_row < sr) ? -1 : 0);
    int dc = (target_col > sc) ? 1 : ((target_col < sc) ? -1 : 0);

    // If no direction, the robot basically aimed at itself. Skip.
    if (dr == 0 && dc == 0 && weapon != grenade) {
        return;
    }

    std::cout << " " << shooter->m_name << " fires weapon!\n";

    // Helper lambda to deal damage to a robot
    auto apply_damage = [&](RobotBase* target, int base_damage) {
        float reduction = target->get_armor() * 0.1f;
        int actual_damage = base_damage - static_cast<int>(base_damage * reduction);
        target->take_damage(actual_damage);
        target->reduce_armor(1);
        std::cout << "   Hits " << target->m_name << " for " << actual_damage << " damage!\n";
    };

    if (weapon == railgun) {
        int r = sr + dr;
        int c = sc + dc;
        int dmg = 10 + (std::rand() % 11); // 10 to 20
        
        while (r >= 0 && r < m_height && c >= 0 && c < m_width) {
            for (RobotBase* target : m_robots) {
                if (target == shooter || target->get_health() <= 0) continue;
                int tr, tc;
                target->get_current_location(tr, tc);
                if (tr == r && tc == c) {
                    apply_damage(target, dmg);
                }
            }
            r += dr;
            c += dc;
        }
    } 
    else if (weapon == hammer) {
        // Hammer only hits an immediately adjacent cell
        if (std::abs(target_row - sr) <= 1 && std::abs(target_col - sc) <= 1) {
            int dmg = 50 + (std::rand() % 11); // 50 to 60
            for (RobotBase* target : m_robots) {
                if (target == shooter || target->get_health() <= 0) continue;
                int tr, tc;
                target->get_current_location(tr, tc);
                if (tr == target_row && tc == target_col) {
                    apply_damage(target, dmg);
                }
            }
        }
    } 
    else if (weapon == grenade) {
        if (shooter->get_grenades() <= 0) return; // Out of ammo
        shooter->decrement_grenades();
        int dmg = 10 + (std::rand() % 31); // 10 to 40
        
        // Hits a 3x3 square centered on the target
        for (RobotBase* target : m_robots) {
            if (target == shooter || target->get_health() <= 0) continue;
            int tr, tc;
            target->get_current_location(tr, tc);
            if (std::abs(tr - target_row) <= 1 && std::abs(tc - target_col) <= 1) {
                apply_damage(target, dmg);
            }
        }
    } 
    else if (weapon == flamethrower) {
        int dmg = 30 + (std::rand() % 21); // 30 to 50
        
        // Similar to the radar logic, get the perpendicular offsets for the 3-wide beam
        int p1_dr = 0, p1_dc = 0, p2_dr = 0, p2_dc = 0;
        if (dr == 0) { p1_dr = -1; p2_dr = 1; } 
        else if (dc == 0) { p1_dc = -1; p2_dc = 1; } 
        else { p1_dr = dr; p1_dc = 0; p2_dr = 0; p2_dc = dc; }

        // Flame goes exactly 4 cells from the robot
        for (int dist = 1; dist <= 4; ++dist) {
            int center_r = sr + (dr * dist);
            int center_c = sc + (dc * dist);

            std::pair<int, int> blast_cells[3] = {
                {center_r, center_c},
                {center_r + p1_dr, center_c + p1_dc},
                {center_r + p2_dr, center_c + p2_dc}
            };

            for (auto& cell : blast_cells) {
                for (RobotBase* target : m_robots) {
                    if (target == shooter || target->get_health() <= 0) continue;
                    int tr, tc;
                    target->get_current_location(tr, tc);
                    if (tr == cell.first && tc == cell.second) {
                        apply_damage(target, dmg);
                    }
                }
            }
        }
    }
}

void Arena::handle_move(RobotBase* robot, int direction, int speed) {
    int curr_r, curr_c;
    robot->get_current_location(curr_r, curr_c);

    // Prevent cheating: cap speed at the robot's actual max speed
    int max_speed = robot->get_move_speed();
    if (speed > max_speed) {
        speed = max_speed;
    }

    int dr = directions[direction].first;
    int dc = directions[direction].second;

    std::cout << " " << robot->m_name << " moving direction " << direction 
              << " for " << speed << " spaces.\n";

    for (int step = 0; step < speed; ++step) {
        int next_r = curr_r + dr;
        int next_c = curr_c + dc;

        // Boundary check
        if (next_r < 0 || next_r >= m_height || next_c < 0 || next_c >= m_width) {
            break; // Hit the wall, stop moving
        }

        // Check for other robots (live or dead)
        bool robot_collision = false;
        for (RobotBase* other : m_robots) {
            if (other == robot) continue;
            int orow, ocol;
            other->get_current_location(orow, ocol);
            if (orow == next_r && ocol == next_c) {
                robot_collision = true;
                break;
            }
        }

        if (robot_collision) {
            break; // Stop prior to the occupied cell
        }

        char cell = m_board[next_r][next_c];

        if (cell == 'M') {
            break; // Stop prior to the mound
        } 
        else if (cell == 'P') {
            // Trapped in the pit!
            curr_r = next_r;
            curr_c = next_c;
            robot->move_to(curr_r, curr_c);
            robot->disable_movement();
            std::cout << "   " << robot->m_name << " fell into a pit at " << curr_r << "," << curr_c << "!\n";
            return; // Movement completely ends
        } 
        else if (cell == 'F') {
            // Move through the flamethrower and take damage
            curr_r = next_r;
            curr_c = next_c;
            
            // Random damage between 30-50
            int dmg = 30 + (std::rand() % 21);
            float armor_reduction = robot->get_armor() * 0.1f;
            int final_dmg = dmg - static_cast<int>(dmg * armor_reduction);
            
            robot->take_damage(final_dmg);
            robot->reduce_armor(1);
            
            std::cout << "   " << robot->m_name << " walked through a flamethrower! Took " 
                      << final_dmg << " damage.\n";
            
            if (robot->get_health() <= 0) {
                robot->move_to(curr_r, curr_c);
                return; // Died mid-movement
            }
        } 
        else {
            // Empty space ('.')
            curr_r = next_r;
            curr_c = next_c;
        }
    }

    // Update final location
    robot->move_to(curr_r, curr_c);
}
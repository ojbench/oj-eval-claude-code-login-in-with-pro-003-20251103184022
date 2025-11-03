#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    bool before_freeze;
};

struct ProblemStatus {
    bool solved;
    int solve_time;
    int wrong_attempts_before_first_success;
    int wrong_attempts_before_freeze;
    int submissions_after_freeze;
    bool frozen;

    ProblemStatus() : solved(false), solve_time(0), wrong_attempts_before_first_success(0),
                      wrong_attempts_before_freeze(0), submissions_after_freeze(0), frozen(false) {}
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int solved_count;
    int penalty_time;
    int ranking;
    vector<int> solve_times; // for tie-breaking

    Team() : solved_count(0), penalty_time(0), ranking(0) {}
};

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> team_order; // for maintaining order
    bool competition_started;
    bool is_frozen;
    int duration_time;
    int problem_count;
    vector<string> problem_names;
    int freeze_time;

    void calculate_team_stats(Team& team, bool include_frozen) {
        team.solved_count = 0;
        team.penalty_time = 0;
        team.solve_times.clear();

        for (auto& p : team.problems) {
            ProblemStatus& ps = p.second;
            if (ps.solved && (!ps.frozen || include_frozen)) {
                team.solved_count++;
                team.penalty_time += ps.solve_time + 20 * ps.wrong_attempts_before_first_success;
                team.solve_times.push_back(ps.solve_time);
            }
        }

        sort(team.solve_times.rbegin(), team.solve_times.rend());
    }

    bool compare_teams(const string& name1, const string& name2, bool include_frozen) {
        Team& t1 = teams[name1];
        Team& t2 = teams[name2];

        calculate_team_stats(t1, include_frozen);
        calculate_team_stats(t2, include_frozen);

        if (t1.solved_count != t2.solved_count) {
            return t1.solved_count > t2.solved_count;
        }
        if (t1.penalty_time != t2.penalty_time) {
            return t1.penalty_time < t2.penalty_time;
        }

        // Compare solve times from largest to smallest
        for (size_t i = 0; i < min(t1.solve_times.size(), t2.solve_times.size()); i++) {
            if (t1.solve_times[i] != t2.solve_times[i]) {
                return t1.solve_times[i] < t2.solve_times[i];
            }
        }

        return name1 < name2;
    }

    void flush_scoreboard() {
        vector<string> sorted_teams = team_order;
        sort(sorted_teams.begin(), sorted_teams.end(), [this](const string& a, const string& b) {
            return compare_teams(a, b, false);
        });

        for (size_t i = 0; i < sorted_teams.size(); i++) {
            teams[sorted_teams[i]].ranking = i + 1;
        }
    }

    void print_scoreboard() {
        vector<string> sorted_teams = team_order;
        sort(sorted_teams.begin(), sorted_teams.end(), [this](const string& a, const string& b) {
            return compare_teams(a, b, false);
        });

        for (auto& team_name : sorted_teams) {
            Team& team = teams[team_name];
            calculate_team_stats(team, false);

            cout << team_name << " " << team.ranking << " "
                 << team.solved_count << " " << team.penalty_time;

            for (auto& pname : problem_names) {
                cout << " ";
                if (team.problems.count(pname)) {
                    ProblemStatus& ps = team.problems[pname];
                    if (ps.frozen) {
                        cout << (ps.wrong_attempts_before_freeze == 0 ? "" : "-")
                             << ps.wrong_attempts_before_freeze << "/"
                             << ps.submissions_after_freeze;
                    } else if (ps.solved) {
                        cout << "+";
                        if (ps.wrong_attempts_before_first_success > 0) {
                            cout << ps.wrong_attempts_before_first_success;
                        }
                    } else {
                        int wrong = ps.wrong_attempts_before_freeze;
                        if (wrong == 0) {
                            cout << ".";
                        } else {
                            cout << "-" << wrong;
                        }
                    }
                } else {
                    cout << ".";
                }
            }
            cout << "\n";
        }
    }

public:
    ICPCSystem() : competition_started(false), is_frozen(false), duration_time(0),
                   problem_count(0), freeze_time(0) {}

    void add_team(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.count(team_name)) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            teams[team_name] = Team();
            teams[team_name].name = team_name;
            team_order.push_back(team_name);
            cout << "[Info]Add successfully.\n";
        }
    }

    void start_competition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            competition_started = true;
            duration_time = duration;
            problem_count = problems;

            for (int i = 0; i < problems; i++) {
                problem_names.push_back(string(1, 'A' + i));
            }

            // Initialize rankings by lexicographic order
            vector<string> sorted_teams = team_order;
            sort(sorted_teams.begin(), sorted_teams.end());
            for (size_t i = 0; i < sorted_teams.size(); i++) {
                teams[sorted_teams[i]].ranking = i + 1;
            }

            cout << "[Info]Competition starts.\n";
        }
    }

    void submit(const string& problem, const string& team_name,
                const string& status, int time) {
        Team& team = teams[team_name];
        ProblemStatus& ps = team.problems[problem];

        Submission sub;
        sub.problem = problem;
        sub.status = status;
        sub.time = time;
        sub.before_freeze = !is_frozen;
        team.submissions.push_back(sub);

        if (is_frozen) {
            // After freeze, if problem was not solved before freeze, mark as frozen
            if (!ps.solved) {
                ps.frozen = true;
                ps.submissions_after_freeze++;
            }
        } else {
            // Before freeze
            if (!ps.solved) {
                if (status == "Accepted") {
                    ps.solved = true;
                    ps.solve_time = time;
                    ps.wrong_attempts_before_first_success = ps.wrong_attempts_before_freeze;
                } else {
                    ps.wrong_attempts_before_freeze++;
                }
            }
        }
    }

    void flush() {
        flush_scoreboard();
        cout << "[Info]Flush scoreboard.\n";
    }

    void freeze() {
        if (is_frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            is_frozen = true;
            freeze_time = 0; // would need to track actual time if needed

            // Mark problems as frozen if not solved
            for (auto& tp : teams) {
                Team& team = tp.second;
                for (auto& pp : team.problems) {
                    ProblemStatus& ps = pp.second;
                    if (!ps.solved) {
                        // Problem should be frozen if it has submissions after freeze
                        // but we'll mark it during submit
                    }
                }
            }

            cout << "[Info]Freeze scoreboard.\n";
        }
    }

    void scroll() {
        if (!is_frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush and print scoreboard
        flush_scoreboard();
        print_scoreboard();

        // Scroll process: unfreeze problems one by one
        while (true) {
            // Get current ranking order
            vector<string> sorted_teams = team_order;
            sort(sorted_teams.begin(), sorted_teams.end(), [this](const string& a, const string& b) {
                return compare_teams(a, b, false);
            });

            // Find lowest ranked team with frozen problems
            string lowest_team = "";
            for (int i = sorted_teams.size() - 1; i >= 0; i--) {
                Team& team = teams[sorted_teams[i]];
                bool has_frozen = false;
                for (auto& pp : team.problems) {
                    if (pp.second.frozen) {
                        has_frozen = true;
                        break;
                    }
                }
                if (has_frozen) {
                    lowest_team = sorted_teams[i];
                    break;
                }
            }

            if (lowest_team.empty()) break;

            // Find smallest problem number that is frozen
            Team& team = teams[lowest_team];
            string unfreeze_problem = "";
            for (auto& pname : problem_names) {
                if (team.problems[pname].frozen) {
                    unfreeze_problem = pname;
                    break;
                }
            }

            int old_rank = team.ranking;

            // Unfreeze and process submissions
            ProblemStatus& ps = team.problems[unfreeze_problem];
            ps.frozen = false;

            // Process frozen submissions
            int additional_wrong_attempts = 0;
            for (auto& sub : team.submissions) {
                if (sub.problem == unfreeze_problem && !sub.before_freeze) {
                    if (!ps.solved) {
                        if (sub.status == "Accepted") {
                            ps.solved = true;
                            ps.solve_time = sub.time;
                            ps.wrong_attempts_before_first_success = ps.wrong_attempts_before_freeze + additional_wrong_attempts;
                        } else {
                            additional_wrong_attempts++;
                        }
                    }
                }
            }
            // Update wrong attempts count (this is for display purposes when not solved)
            if (!ps.solved) {
                ps.wrong_attempts_before_freeze += additional_wrong_attempts;
            }

            // Save old rankings before recalculating
            map<string, int> old_rankings;
            for (auto& name : sorted_teams) {
                old_rankings[name] = teams[name].ranking;
            }

            // Recalculate rankings
            flush_scoreboard();

            int new_rank = team.ranking;

            // If ranking changed, output the change
            if (new_rank < old_rank) {
                // Find the team that was at the new_rank position before this change
                // This is the team that lowest_team replaced
                string replaced_team = "";
                for (auto& name : sorted_teams) {
                    if (old_rankings[name] == new_rank && name != lowest_team) {
                        replaced_team = name;
                        break;
                    }
                }

                calculate_team_stats(team, false);
                cout << lowest_team << " " << replaced_team << " "
                     << team.solved_count << " " << team.penalty_time << "\n";
            }
        }

        // Print final scoreboard
        print_scoreboard();

        is_frozen = false;

        // Reset frozen submission counts for all teams
        for (auto& tp : teams) {
            Team& t = tp.second;
            for (auto& pp : t.problems) {
                ProblemStatus& ps = pp.second;
                ps.submissions_after_freeze = 0;
            }
        }
    }

    void query_ranking(const string& team_name) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query ranking.\n";
            if (is_frozen) {
                cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            }
            cout << team_name << " NOW AT RANKING " << teams[team_name].ranking << "\n";
        }
    }

    void query_submission(const string& team_name, const string& problem, const string& status) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query submission.\n";

            Team& team = teams[team_name];
            Submission* found = nullptr;

            for (int i = team.submissions.size() - 1; i >= 0; i--) {
                Submission& sub = team.submissions[i];
                bool match = true;

                if (problem != "ALL" && sub.problem != problem) {
                    match = false;
                }
                if (status != "ALL" && sub.status != status) {
                    match = false;
                }

                if (match) {
                    found = &sub;
                    break;
                }
            }

            if (found) {
                cout << team_name << " " << found->problem << " "
                     << found->status << " " << found->time << "\n";
            } else {
                cout << "Cannot find any submission.\n";
            }
        }
    }

    void end_competition() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.add_team(team_name);
        } else if (command == "START") {
            string duration_str, problem_str;
            int duration, problems;
            iss >> duration_str >> duration >> problem_str >> problems;
            system.start_competition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, by, team_name, with, status, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time;
            system.submit(problem, team_name, status, time);
        } else if (command == "FLUSH") {
            system.flush();
        } else if (command == "FREEZE") {
            system.freeze();
        } else if (command == "SCROLL") {
            system.scroll();
        } else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.query_ranking(team_name);
        } else if (command == "QUERY_SUBMISSION") {
            string team_name, where, problem_eq, and_str, status_eq;
            iss >> team_name >> where >> problem_eq >> and_str >> status_eq;

            string problem = problem_eq.substr(8); // skip "PROBLEM="
            string status = status_eq.substr(7);   // skip "STATUS="

            system.query_submission(team_name, problem, status);
        } else if (command == "END") {
            system.end_competition();
            break;
        }
    }

    return 0;
}

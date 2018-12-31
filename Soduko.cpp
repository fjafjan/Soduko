#include "util.h"

#include <vector>
#include <stdio.h>
#include <string.h>
#include <ctime>

#include <map>
#include <cassert>
#include <algorithm>

// Skeleton class for solving Soduko puzzle.
// Implemented
// - Read (depending on the formatting of the input)
// - Solve (At least what I have tested, I got to the point that the puzzle was not determined) 
//
// Main tasks remaining
// - Refactoring code to be a little bit more clear cut.
// - Validate if the Soduko is determined or not (if there are multiple solutions that are valid.)

// Actually the order should be invariant, so instead it should be the reversal that should be changed.
// Instead of reverting the LATEST, I should revert one of the picks that has the most options remaining, 
// if the largest options remaining is 1, then we are boned. 

// So above was wrong, or sort of. It is true that the order of the picks IS invariant, however each pick
// is conditionally correct on the other picks. So when evaluating if we have done something before, we 
// should evaluate it based on a hash of the current state.

// so TODO today which should get it working, is to hash the current state into an array of bytes. So the 
// total size of this state will be 9 * 9 * 9, i.e 729 bits. Not that large in itself but too larger to fit
// into a single number right? I can think about what is the neatest way of compressing that, but realistically
// it will have to be an array or string of some sort.

/// Just a little bit of logging for what is happening.
#define VERBOSE 0 

/// When debugging we want to log a lot.
#define DEBUG 0

static int ToIndex(int x, int y) { return (y*9) + x; }

static void ToPos(int index, int & x, int & y) { x = index%9; y = index/9; }

static int ToPosX(int index) { return index%9; }

static int ToPosY(int index) { return index/9; }
class Limiter {
public:
    Limiter() {
        values.resize(9, -1);
    }

    bool HasVal(int val) {return values[val] > 0; }
    bool Add(int val) {
        // Lets add an assert here for now...
        assert(!HasVal(val));
        if (HasVal(val)) return false;
        values[val] = 1;
        return true;
    };

    void Remove(int val) { values[val] = -1; }
    bool IsFull() {
        for (auto & val : values) {
            if (val < 0) return false;
        }
        return true;
    }

    void Print(const char * name) {
        printf("Limits for %s are", name);
        for (int i = 0; i < 9; i++) {
            printf("%d : %d \n", i, values[i]);
        }
    }
private:
    std::vector<int> values;
};


class Soduko {
public:    
    Soduko();
    void Read(const char * input);

    bool Solve(); // If we manage to solve it
    bool IsSolved();

    void PrintStatus(); // Print out remaining squares to be filled and the options that remain.
    void Print(bool human_readable=true);

    int GetVal(int pos_x, int pos_y) { return values_[pos_y*9 + pos_x]; };
    bool SetVal(int pos_x, int pos_y, int val, bool set_hard = false);
    bool ResetVal(int pos_x, int pos_y); // Set back to undecided.

    // Very simple class representing a value at a certain psotion.
    class Pick {
        public:
        // Default constructor should never be used...
        Pick() : pos_x_(-1), pos_y_(-1), val_(-1), index_(-1) {}
        Pick(int pos_x, int pos_y, int val) : pos_x_(pos_x), pos_y_(pos_y), val_(val), index_(ToIndex(pos_x, pos_y)) {}
        Pick(int index, int val) : pos_x_(ToPosX(index)), pos_y_(ToPosY(index)), val_(val), index_(index) {}

        // Copy operator.
        Pick(const Pick & other) : pos_x_(other.pos_x_), pos_y_(other.pos_y_), val_(other.val_), index_(other.index_) {}
        // Move operator
        Pick& operator=(const Pick& other) {
            *(const_cast<int*>(&pos_x_)) = other.pos_x_;
            *(const_cast<int*>(&pos_y_)) = other.pos_y_;
            *(const_cast<int*>(&val_)) = other.val_;
            *(const_cast<int*>(&index_)) = other.index_;
            return *this;
        }

        bool operator==(Pick & other) { return (index_ == other.index_ && val_ == other.val_);}  

        const int pos_x_;
        const int pos_y_;
        const int val_;
        const int index_;
    };

    class State {
        public:
        State() { set_options.resize(81 * 9, false); }

        bool operator<(const State & other) const
        {
            assert(other.set_options.size() == set_options.size()); 
            for(int i = 0; i < set_options.size(); i++) {
                if (set_options[i] != other.set_options[i]) {
                    return (set_options[i] < other.set_options[i]);
                }
            }
            return false;
        } 
        std::vector<bool> set_options;
    };

private:
    State ToState();
    Pick FindPick(std::vector<Pick> & fails);
    int FindRevertPick(std::vector<Pick> & picks); // Finds which of currently tested picks to revert based on most options remaining.
    bool Test(int pos_x, int pos_y, int val); // Check if putting a number is possible.
    bool Forget(Pick pick); // Revert an earlier attempt.
    // Finds for each unassigned square the number of options for that square.Get
    std::vector<int> GetPossibleValues(int pos_x, int pos_y);
    std::vector<int> GetPossibleValues(int index) {return GetPossibleValues(ToPosX(index), ToPosY(index)); };
    
    void GetNumberOfOptions(std::vector<int> & target, std::vector<int> & options_out); 
    Pick RevertPick(std::vector<Pick> & picks); // Find a pick to revert.
    // Representing the fact that we can only have one of each
    // number in each row/column/block
    Limiter * GetRowLimiter(int pos_x, int pos_y) { return &(row_limits[pos_y]); }
    Limiter * GetColLimiter(int pos_x, int pos_y) { return &(col_limits[pos_x]); }
    Limiter * GetBlockLimiter(int pos_x, int pos_y) { 
        int block_x = pos_x / 3;
        int block_y = pos_y / 3;
        return &(block_limits[block_y * 3 + block_x]); 
    }

    std::vector<int> values_; // The current state of the soduko board
    std::vector<bool> set_hard_; // The values read are never changed. 
    // std::vector< std::vector<int> >has_tested_; // note for each position if we have tested a certain value there
    std::map<Soduko::State, bool> explored_states;

    std::vector<Limiter> row_limits, col_limits, block_limits;
    std::vector<int> unassigned_vals;
    Soduko::Pick fail_pick;
};


Soduko::Soduko() {
    values_.resize(81, -1);
    set_hard_.resize(81, false);
    row_limits.resize(9);
    col_limits.resize(9);
    block_limits.resize(9);
    fail_pick = Pick(-1, -1);
}

// Prints out the current state of the Soduko for visualization
void Soduko::Print(bool human_readable)
{
    printf("Soduko state is \n");
    for (int row = 0; row < 9; row++) {
        for (int block = 0; block < 3; block++) {
            if (human_readable) {
                printf("%d%d%d ", GetVal(block*3, row)+1, GetVal(block*3 + 1, row)+1, GetVal(block*3 + 2, row)+1);
            } else {
                printf("%d%d%d ", GetVal(block*3, row), GetVal(block*3 + 1, row), GetVal(block*3 + 2, row));
            }
        }
        printf("\n");
    }
}

Soduko::State Soduko::ToState()
{
    Soduko::State ret;

    for (int pos_x = 0; pos_x < 9; pos_x++) {
        for (int pos_y = 0; pos_y < 9; pos_y++) {
            if (GetVal(pos_x, pos_y) >= 0) {
                int index = ToIndex(pos_x, pos_y);
                int val = GetVal(pos_x, pos_y);
                ret.set_options[(index * 9) + val] = true;
            }
        }
    }
    return ret;
}

void Soduko::Read(const char * input)
{
    if (!input) {
        printf("Invalid nullpointer input!");
        return;
    }
    int pos_x = 0;
    int pos_y = 0;
    for (int i = 0; i < strlen(input); i++) {
        if (input[i] == ' ') continue; // Spaces are for formatting
        if (input[i] == '\n') {
            printf("Found newline \n");
            pos_x = 0;
            pos_y += 1;
            continue;
        }
        int val = int(input[i]) - int('1'); // We want 0 indexing.
        if (val < 0 || val > 8) { // Values other than number are unsolved
            if (DEBUG) printf("Setting position (%d %d) to unassigned \n", pos_x, pos_y);
            unassigned_vals.push_back(ToIndex(pos_x, pos_y));
            pos_x++;
            continue;
        }
        // printf("Got value %d at (%d, %d) \n", val, pos_x, pos_y);
        SetVal(pos_x, pos_y, val, true);
        pos_x++;
    }
    printf("Parsed %d values", strlen(input));
}

void Soduko::PrintStatus()
{
    for (auto & index : unassigned_vals) {
        int pos_x, pos_y;
        ToPos(index, pos_x, pos_y);
        printf("Remaining values to be tested for position (%d, %d) are: ", pos_x, pos_y);
        std::vector<int> posssible_values = GetPossibleValues(pos_x, pos_y);
        for (auto & v : posssible_values) {
            printf("%d, ", v+1);
        }
        printf("\n");
    }
}


void Soduko::GetNumberOfOptions(std::vector<int> & target, std::vector<int> & options_out)
{
    options_out.clear();
    options_out.resize(target.size(), 0);
    for (int val_idx = 0; val_idx < target.size(); val_idx++) {
        int index = target[val_idx];
        std::vector<int> options = GetPossibleValues(ToPosX(index), ToPosY(index));
        options_out[val_idx] = options.size();
    }    
}

std::vector<int> Soduko::GetPossibleValues(int pos_x, int pos_y) 
{
    std::vector<int> ret;
    for (int i = 0; i < 9; i++) {
        int val = i;
        if (GetRowLimiter(pos_x, pos_y)->HasVal(val)) continue; 
        if (GetColLimiter(pos_x, pos_y)->HasVal(val)) continue; 
        if (GetBlockLimiter(pos_x, pos_y)->HasVal(val)) continue;
        ret.push_back(val); 
    }
    return ret;
}


// Find a square that we can fill in.
Soduko::Pick Soduko::FindPick(std::vector<Soduko::Pick> & fails)
{
    if (DEBUG) printf("Trying to find pick\n");
    std::vector<int> pick_options;
    GetNumberOfOptions(unassigned_vals, pick_options);
    // I think the correct thing to do here is to sort the vector in decreasing order
    // of picks. But maybe also realistically if we fail on a 1 long we will enter some other
    // fail state.
    int min_options, min_idx;
    ListMin(pick_options, &min_options, &min_idx);
    if (DEBUG) printf("Fewest picks remaining was %d with %d options\n", unassigned_vals[min_idx], min_options);
    if (min_options == 0) {
        if (DEBUG) printf("Exhausted picks\n");
        return fail_pick;
    } else {
        int picked_idx = unassigned_vals[min_idx];
        int pos_x, pos_y;
        ToPos(picked_idx, pos_x, pos_y);
        for (int val = 0; val < 9; val++) {
            Soduko::State next_state = ToState();
            next_state.set_options[(picked_idx * 9) + val] = true;
            if (explored_states.count(next_state) == 0) { 
            // if (has_tested_[picked_idx][val] < 0) {
                if (DEBUG) printf("Exploring new state\n");
                explored_states[next_state] = true;

                if (DEBUG) printf("Untested value %d at pos (%d %d)\n", val+1, pos_x, pos_y);
                if (Test(pos_x, pos_y, val)) {
                    if (DEBUG) printf("Testing new value %d at pos (%d %d) \n", val+1, pos_x, pos_y);
                    // Remove this value from the list of unassigned values.
                    unassigned_vals.erase(std::remove(unassigned_vals.begin(), unassigned_vals.end(), picked_idx), unassigned_vals.end());
                    return Pick(pos_x, pos_y, val);
                }
            } else {
                if (DEBUG) printf("Ignoring this attempt since we have been exactly here before");
            }
        }
    }

    if (VERBOSE) printf("Unable to find any valid pick");
    return fail_pick;
}

int Soduko::FindRevertPick(std::vector<Pick> & picks)
{
    if (DEBUG) printf("Trying to find revert \n");
    std::vector<int> pick_options;
    std::vector<int> pick_indices;
    for (auto & p : picks) pick_indices.push_back(p.index_);
    GetNumberOfOptions(pick_indices, pick_options);
    // I think the correct thing to do here is to sort the vector in decreasing order
    // of picks. But maybe also realistically if we fail on a 1 long we will enter some other
    // fail state.
    if (DEBUG) printf("Pick options is are: ");
    if (DEBUG) for (auto p : pick_options) printf("%d ", p);
    if (DEBUG) printf("\n");

    int max_options, max_idx;
    ListMax(pick_options, &max_options, &max_idx);
    if (DEBUG) printf("Most options remaining was %d options at index %d\n", max_options, max_idx);
    if (DEBUG) printf("Returning pick: val=%d, pos=(%d, %d)", picks[max_idx].val_, picks[max_idx].pos_x_, picks[max_idx].pos_y_);
    return max_idx;
}

bool Soduko::Solve()
{
    std::vector<Pick> picks;
    std::vector<Pick> fails;
    int pos_x, pos_y;
    int max_iter = 3000; // Just for safety...
    int iter = 0;
    // So we are happy with a DFS search I think.
    // So I think what we want is first for the pick function to be
    // isolated so that it doesn't affect the state as clearly?
    // And then we need a reversal mechanism. So how should that work?
    // Well when we try to pick a square, we will be unable to do so. We should then
    // pop back, and try to pick a new square. 
    // In the simplest case, we simply pick a new square each time and it all works out, 
    // in the harder case we have to step back a lot and it is inefficient but it should 
    // still work. 
    printf("Starting to solve\n");
    while (true) {
        if (iter++ > max_iter) break;
        Pick new_pick = FindPick(fails);
        if (new_pick == fail_pick) {
            if (VERBOSE) printf("Stepping back after failed pick\n");
            if (picks.empty()) {
                if (VERBOSE) printf("No more steps to back, failed solve\n");
                return false;
            }
            // Pick optional = RevertPick(picks);
            // Pick last_pick = picks.back();
            // We want to choose one of our chosen picks with the most options to undo and try another one of those.
            int revert_idx = FindRevertPick(picks);
            Pick revert_pick = picks[revert_idx];
            Forget(revert_pick);
            fails.push_back(revert_pick);
            picks.erase(picks.begin() + revert_idx);
        } else {
            if (VERBOSE) printf("Push back pick (%d : (%d %d)) to list\n", new_pick.val_, new_pick.pos_x_, new_pick.pos_y_);
            picks.push_back(new_pick);
        }

        if (IsSolved()) {
            printf("We did manage to solve it somehow after %d iterations..\n", iter);
            return true;
        } else if (unassigned_vals.size() == 0) {
            printf("ERROR No unassigned values yet we are not solved. \n");
            return false;
        }
        if (VERBOSE) printf("Iteration %d\n ", iter);

        /// Prints the board and the current remaining options.
        if (DEBUG) Print();
        if (DEBUG) PrintStatus();
    }

    printf("FAILED: Ran out of iterations \n");
    return false;

}

bool Soduko::SetVal(int pos_x, int pos_y, int val, bool set_hard)
{ 
    // Update value and all limitors.
    if (DEBUG) printf("Setting value %d at position (%d %d)\n", val, pos_x, pos_y);
    int index = ToIndex(pos_x, pos_y); // Row major indexing.
    if (set_hard_[index]) return false; // Once set hard the values is not allowed to change
    GetColLimiter(pos_x, pos_y)->Add(val);
    GetRowLimiter(pos_x, pos_y)->Add(val);
    GetBlockLimiter(pos_x, pos_y)->Add(val);
    values_[index] = val;
    set_hard_[index] = set_hard;
    return true;
}

bool Soduko::ResetVal(int pos_x, int pos_y)
{
    // Update value and all limitors.
    int index = ToIndex(pos_x, pos_y); // Row major indexing.
    if (set_hard_[index]) return false; // Once set hard the values is not allowed to change
    int val = values_[index];
    if (DEBUG) printf("Resetting value %d at position (%d %d)\n",val, pos_x, pos_y);
    if (val < 0) {
        printf("ERROR: Resetting value that was already unset!");
        return false;
    }
    GetColLimiter(pos_x, pos_y)->Remove(val);
    GetRowLimiter(pos_x, pos_y)->Remove(val);
    GetBlockLimiter(pos_x, pos_y)->Remove(val);
    values_[index] = -1;
    return true;
}

bool Soduko::Test(int pos_x, int pos_y, int val)
{
    if (DEBUG) printf("Checking if we can put value %d at pos (%d %d)\n", val, pos_x, pos_y);
    int index = ToIndex(pos_x, pos_y);
    if (set_hard_[pos_y*9 + pos_x]) return false;

    // Any limiting constraint?
    if (GetRowLimiter(pos_x, pos_y)->HasVal(val)) {
        if (DEBUG) printf("Cant place in row %d\n", pos_y);
        return false;
    }
    if (GetColLimiter(pos_x, pos_y)->HasVal(val)) {
        if (DEBUG) printf("Can't place in col %d", pos_x);
        return false;
    } 
    if (GetBlockLimiter(pos_x, pos_y)->HasVal(val)) {
        if (DEBUG) printf("Can't place in block %d", pos_x/3, pos_y/3);
        return false;
    }
    // Lets test it 
    if (DEBUG) printf("We can!\n");
    SetVal(pos_x, pos_y, val);
    return true; // Ready for test
}

bool Soduko::Forget(Soduko::Pick pick)
{
    // has_tested_[pick.pos_y_*9 + pick.pos_x_][pick.val_] = -1;
    unassigned_vals.push_back(pick.index_);
    ResetVal(pick.pos_x_, pick.pos_y_);
    return true;
}

bool Soduko::IsSolved()
{
    for (int i = 0; i < 9; i++) {
        if (!row_limits[i].IsFull()) {
            return false;
        }
        if (!col_limits[i].IsFull()) {
            return false;
        }
        if (!block_limits[i].IsFull()) {
            return false;
        }
    }
    return true;
}

int main ()
{
    Soduko my_soduko_solver;
    static const char solved_soduko[] = "123 456 789\n 456 789 123\n 789 123 456\n 234 567 891\n 567 891 234\n 891 234 567\n 345 678 912\n 912 345 678\n 678 912 345";
    static const char test_soduko[] =   "x23 4xx 789\n xx6 xx9 12x\n xxx 1x3 4xx\n xx4 567 xx1\n 5xx 8x1 xxx\n xx1 234 567\n 3x5 67x 9xx\n 9xx 345 678\n 678 xx2 xx5";
    static const char harder_soduko[] = "x23 4xx xx9\n xx6 xx9 12x\n xxx 1x3 4xx\n xx4 5x7 xx1\n 5xx 8x1 xxx\n xx1 23x xx7\n 3x5 x7x 9xx\n 9xx 3xx 67x\n 678 xx2 xx5";
    printf("This is my first compiled C++ program at home");
    my_soduko_solver.Read(harder_soduko);
    my_soduko_solver.Print();
    std::clock_t c_start = std::clock();
    if (my_soduko_solver.IsSolved()) {
        printf("Test soduko is already solved\n");
    } else {
        printf("It is not solved..\n");
        if (my_soduko_solver.Solve()) {
            printf("Success!\n");
        } else {
            printf("Failure :(\n");
        }
    }
    std::clock_t c_end = std::clock();
    my_soduko_solver.Print();
    printf("Solving took: %f ms\n", 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC );
    return 0;
}
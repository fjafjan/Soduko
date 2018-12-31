// Lets write this list of booleans that we can use to keep track of the state.
#include <vector>
#include <stdio.h>

static void logv(const char * message, void * args)
{
    printf("%s\n", message, args);
}

template <typename T> 
void ListMin(std::vector<T> & v, T * min_val, int * min_idx )
{
    if (v.empty()) return;
    *min_val = v[0];
    *min_idx = 0;
    for (int i = 0; i < v.size(); i++) {
        if (v[i] < (*min_val)) {
            *min_val = v[i];
            *min_idx = i;
        }
    }
} 

template <typename T> 
void ListMax(std::vector<T> & v, T * max_val, int * max_idx )
{
    if (v.empty()) {
        printf("ERROR: Trying to find max of empty vector");
        return;
    } 
    *max_val = v[0];
    *max_idx = 0;
    for (int i = 0; i < v.size(); i++) {
        if (v[i] > (*max_val)) {
            *max_val = v[i];
            *max_idx = i;
        }
    }
}

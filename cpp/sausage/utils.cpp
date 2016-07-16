#include "utils.h"

#define print(var) std::cout<<#var"= "<<var<<std::endl
int tohash(int len, int e1, int e2, int e3, int e4, int e5) {
    int res = 1;

    res += e1;
    res += (res << 10);
    res ^= (res >> 6);

    res += e2;
    res += (res << 10);
    res ^= (res >> 6);

    res += e3;
    res += (res << 10);
    res ^= (res >> 6);

    res += e4;
    res += (res << 10);
    res ^= (res >> 6);

    res += e5;
    res += (res << 10);
    res ^= (res >> 6);

    res += (res << 3);
    res ^= (res >> 11);
    res += (res << 15);
    return (abs(res) % len);
}

int tohash(int len, initializer_list<int> arr) {
    int res = 1;
    for (auto iter = arr.begin(); iter != arr.end(); ++iter) {
        res += *iter;
        res += (res << 10);
        res ^= (res >> 6);
    }
    res += (res << 3);
    res ^= (res >> 11);
    res += (res << 15);
    return (abs(res) % len);
}

vector<string> split(string& line, char delim) {
    istringstream iss(line); string tmp; vector<string> res;
    while (getline(iss, tmp, delim)) res.push_back(tmp);
    return res;
}


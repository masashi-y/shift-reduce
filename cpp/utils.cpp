#include "utils.h"

#define print(var) std::cout<<#var"= "<<var<<std::endl
int tohash(int len, int e1, int e2, int e3, int e4, int e5) {
    int res = 0;

    // print(e1);
    // print(e2);
    // print(e3);
    // print(e4);
    // print(e5);

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

// int tohash(int len, initializer_list<int> arr) {
//     int res = 0;
//     for (auto iter = arr.begin(); iter != arr.end(); ++iter) {
//         cout << (*iter) << endl;
//         res += *iter;
//         res += (res << 10);
//         res ^= (res >> 6);
//     }
//     res += (res << 3);
//     res ^= (res >> 11);
//     res += (res << 15);
//     return (abs(res) % len);
// }

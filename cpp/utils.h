#ifndef SHIFT_REDUCE_UTILS_
#define SHIFT_REDUCE_UTILS_

#include <limits>
#include <initializer_list>
#include <stdlib.h>
#include <iostream>
#include <stdarg.h>

using namespace std;

const double doubleMin = -100000000000.0;

// int tohash(int len, initializer_list<int> arr);
int tohash(int len, int e1=0, int e2=0, int e3=0, int e4=0, int e5=0);

template<typename T>
int argmax(T* arr, int nActions) {
    int maxi = 0; T maxv = doubleMin;
    // cout << maxv << endl;
    for (int i = 0; i < nActions; i++) {
        if (arr[i] >= maxv) {
            maxi = i;
            maxv = arr[i];
        }
    }
    return maxi;
}

template<class T>
void shuffle(T arr[],int size) {
    for(int i = 0;i < size; i++) {
        int j = rand()%size;
        T t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }
}

#endif

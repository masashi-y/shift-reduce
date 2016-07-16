#ifndef SHIFT_REDUCE_UTILS_
#define SHIFT_REDUCE_UTILS_

#include <limits>
#include <sstream>
#include <initializer_list>
#include <stdlib.h>
#include <iostream>
#include <vector>

using namespace std;

const double doubleMin = numeric_limits<double>::lowest();

int tohash(int len, initializer_list<int> arr);
int tohash(int len, int e1=-1, int e2=-1, int e3=-1, int e4=-1, int e5=-1);
vector<string> split(string& line, char delim);


template<typename T>
int argmax(T* arr, int nActions) {
    int maxi = 0;
    T maxv = numeric_limits<T>::lowest();
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

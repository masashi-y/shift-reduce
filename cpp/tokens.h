#ifndef SHIFT_REDUCE_TOKEN_
#define SHIFT_REDUCE_TOKEN_

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "utils.h"

using namespace std;

struct Word
{
    int w;
    int p;
    int l;
    int head;
    string word;
    string pos;
    string label;

    string toConll();
    void init(string& _word, string& _pos, int _head, string& _label);
    Word(string& _word, string& _pos, string& _head, string& _label);
    Word(vector<string>& line);
    Word();
};

void toConll(ostream& os, vector<Word>& sent);
void toConll(vector<Word>& sent);

int getOrAdd(unordered_map<string,int>& dict, string& key);

vector<vector<Word> > readConll(const string& file);

void initFormat();

#endif

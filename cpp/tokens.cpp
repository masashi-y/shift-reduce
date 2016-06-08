#include "tokens.h"

using namespace std;

#define print(var) std::cout<<#var"= "<<var<<std::endl
unordered_map<string,int> words;
unordered_map<string,int> tags;
unordered_map<string,int> labels;
unordered_map<string,int> format;

string root = "root";

int getOrAdd(unordered_map<string,int>& dict, string& key) {
    int id = dict[key];
    if (id == 0) {
        id = dict.size() + 1;
        dict[key] = id;
    }
    return id;
}

Word::Word(string& _word, string& _pos, string& _head, string& _label) {
    init(_word, _pos, stoi(_head), _label);
}
Word::Word() { init(root, root, 0, root); }

Word::Word(vector<string>& line) {
    init(line[format["word"]],
         line[format["pos"]],
         stoi(line[format["head"]]),
         line[format["label"]]);
}

void Word::init(string& _word, string& _pos, int _head, string& _label) {
    word  = _word;
    pos   = _pos;
    head  = _head;
    label = _label;
    w = getOrAdd(words, word);
    p = getOrAdd(tags, pos);
    l = getOrAdd(labels, label);
}

string Word::toConll() {
    ostringstream os;
    os << word  << "\t"
       << "_"   << "\t"
       << pos   << "\t"
       << pos   << "\t"
       << "_"   << "\t"
       << head  << "\t"
       << label << "\t"
       << "_"   << "\t"
       << "_";
    return os.str();
}

void toConll(ostream& os, vector<Word>& sent) {
    for (unsigned i = 1; i < sent.size(); i++)
        os << i << "\t" << sent[i].toConll() << endl;
    os << endl;
}
void toConll(vector<Word>& sent) { toConll(cout, sent); }

vector<vector<Word> > readConll(const string& file) {
    initFormat();
    ifstream conllFile(file);
    string line;
    Word root = Word();
    vector< vector<Word> > res;
    vector<Word> sent;
    sent.push_back(root);
    while (getline(conllFile, line)) {
        if (line.size() == 1) {
            res.push_back(sent);
            sent.clear();
            sent.push_back(root);
            continue;
        }
        vector<string> tmp = split(line, '\t');
        sent.push_back(Word(tmp));
    }
    return res;
}

void initFormat() {
    format["word"]  = 1;
    format["pos"]   = 3;
    format["head"]  = 6;
    format["label"] = 7;
}

int* getHeads(vector<Word>& sent) {
    int* res = new int[sent.size()];
    for (int i = 0; i < sent.size(); i++) {
        res[i] = sent[i].head;
    }
    return res;
}

// int main() {
//     const string file = "wsj_23.conll";
//     auto res = readConll(file);
//
//     for (unsigned i = 0; i < res.size(); i++) {
//         // toConll(cout, res[i]);
//         for (int j = 0; j < res[i].size(); j++) {
//             print(res[i][j].w);
//             print(res[i][j].p);
//             print(res[i][j].l);
//         }
//     }
//
//     // for (auto iter = words.begin(); iter != words.end(); iter++) {
//     //     cout << iter->first << " => " << iter->second << endl;
//     // }
// }

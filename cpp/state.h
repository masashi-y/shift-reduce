#ifndef SHIFT_REDUCE_STATE_
#define SHIFT_REDUCE_STATE_

#include <string.h>
#include "tokens.h"
#include "utils.h"

#define INITEDGE() (sentSize+1)*maxdir*maxorder+maxdir*maxorder+maxorder
#define T(...) feat[i]=tohash(dim, i++, __VA_ARGS__, -1000);

const int LeftArc  = 0;
const int RightArc = 1;
const int Shift    = 2;
const int Reduce   = 3;
const int NOACTION = -1;
const int nActions = 4;

inline const string actionToString(int action) {
    return action == 0 ? "LeftArc" :
           action == 1 ? "RightArc" :
           action == 2 ? "Shift" :
           action == 3 ? "Reduce" : "NONE";
}

const int L = 1;
const int R = 2;
const int H = 3;
const int maxdir   = 3;
const int maxorder = 2;

inline int context(int h, const int d, int o) {
    return h * maxdir * maxorder + d * maxorder + o;
}


class State
{
public:
    State() {};
    State(vector<Word>* sent);
    State(State& ro);
    ~State();
    State* nextState();
    string to_string();
    int getStep() { return step; }
    double getScore() { return score; }
    int** getFeat() { return &feat; }
    int getPrevAct() { return prevact; }
    vector<Word>* getSent() { return sent; }
    // void setPrevState(State* st) { prev = st; }
    int getFeatSize() { return featSize; }
    State* getPrevState() { return prev; }
    State* transit(int action);
    const int goldAction();
    const int predAction();
    void toConll(ostream& os);
    void toConll();
    State** toArray();
    int* heads();
    bool isFinal;
private:
    Word& indToWord(int i);
    void generateFeature();
    bool bufferIsEmpty() { return buffer >= sentSize; }
    bool hasHead(int i) { return edges[context(i,H,1)] != -1; };
    bool* validActions();

    int step;
    double score;
    vector<int> stack;
    int buffer;
    int* edges;
    vector<Word>* sent;
    // Model model;
    State *prev;
    int prevact;
    int* feat;
    int sentSize;
    int featSize;
};


#endif

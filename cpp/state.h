#ifndef SHIFT_REDUCE_STATE_
#define SHIFT_REDUCE_STATE_

#include <string.h>
#include "tokens.h"
#include "utils.h"

#define INITEDGE() (sentSize+1)*MAXDIR*MAXORDER+MAXDIR*MAXORDER+MAXORDER
#define T(...) feat[i]=tohash(dim, {i, __VA_ARGS__}); i++;
// #define T(...) feat[i]=tohash(dim, i++, __VA_ARGS__);

const int LeftArc  = 0;
const int RightArc = 1;
const int Shift    = 2;
const int Reduce   = 3;
const int nActions = 4;
const int NOACTION = -1;

inline const string actionToString(int action) {
    return action == 0 ? "LeftArc" :
           action == 1 ? "RightArc" :
           action == 2 ? "Shift" :
           action == 3 ? "Reduce" : "NONE";
}

const int L = 1;
const int R = 2;
const int H = 3;
const int MAXDIR   = 3;
const int MAXORDER = 2;

inline int context(int h, const int d, int o) {
    return h * MAXDIR * MAXORDER + d * MAXORDER + o;
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
    int*& getFeat() { return feat; }
    int getPrevAct() { return prevact; }
    vector<Word>* getSent() { return sent; }
    int getFeatSize() { return featSize; }
    State* getPrevState() { return prev; }
    State* transit(int action, double score);
    double goldAction(int* action);
    double predAction(int* action);
    void toConll(ostream& os);
    void toConll();
    State** toArray();
    int* heads();
    bool isFinal;
    vector<State*> expandPred();
    vector<State*> expandGold();
    void registerState() {
        stateBuffer[nStateBuffer] = this;
        nStateBuffer++;
    }
    static void clearStates() {
        for (int i = 0; i < nStateBuffer; i++)
            delete stateBuffer[i];
        nStateBuffer = 0;
    }
    Word& wordAt(int i) { return (*sent)[i]; }
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
    State *prev;
    int prevact;
    int* feat;
    int sentSize;
    int featSize;
    static State* stateBuffer[];
    static int nStateBuffer;
};


#endif

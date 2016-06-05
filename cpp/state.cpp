#include "state.h"
// #define NODEBUG
#include "assert.h"
#include <iomanip>
#include <chrono>

#define print(var) std::cout<<#var"= "<<var<<std::endl

const int dim = 1 << 26;
double (*weight)[dim];
vector<vector<Word> > conll;
int UASDen = 0;
int UASNum = 0;

State::State(vector<Word>* _sent) :
    step(0), score(0.0), stack(0), buffer(1), isFinal(false)
{
    stack.push_back(0);
    sent = _sent;
    sentSize = sent->size();
    int N = INITEDGE();
    edges = new int[N];
    for (int i = 0; i < N; i++) { edges[i] = -1; }
    feat = nullptr;
    prev = nullptr;
}

State::~State() {
    // cout << "~State in " << step << "step " << this << endl;
    delete[] edges, feat;
    if (step > 1) delete prev; /////////////////
}

State::State(State& ro):
    step(ro.step+1), score(ro.score), stack(ro.stack), sent(ro.sent),
    buffer(ro.buffer), prev(&ro), sentSize(ro.sentSize), isFinal(false)
{
    int N = INITEDGE();
    edges = new int[N];
    memcpy(edges, ro.edges, sizeof(int)*N);
    feat = nullptr;
}

State* State::nextState() {
    State* st = new State(*this);
    st->prev = this;
    return st;
}

bool* State::validActions() {
    bool* actions = new bool[nActions];
    for (int i = 0; i < nActions; i++) { actions[i] = false; }

    if (!bufferIsEmpty()) {
        actions[Shift] = true;
        if (!stack.empty()) {
            actions[RightArc] = true;
            if (stack.back() != 0 && !hasHead(stack.back())) {
                actions[LeftArc] = true;
            }
        }
    }
    if (stack.size() > 1 && hasHead(stack.back())) {
        actions[Reduce] = true;
    }
    return actions;
}

const int State::goldAction() {
    bool* valid = validActions();
    int res = NOACTION;
    if (false) {
    }else if (valid[LeftArc] && (*sent)[stack.back()].head == buffer) {
        res = LeftArc;
    }else if (valid[RightArc] && (*sent)[buffer].head == stack.back()) {
        res = RightArc;
    }else if (valid[Reduce]) {
        if (bufferIsEmpty()) {
            res = Reduce;
        } else {
            // check if s0 has gold child in the buffer
            bool hasChild = false;
            for (int i = buffer; i < sentSize; i++) {
                if  ((*sent)[i].head == stack.back()) hasChild = true;
            }
            if (!hasChild) res = Reduce;
            else if (valid[Shift]) res = Shift;
        }
    }else if (valid[Shift]) {
        res = Shift;
    } else {
        // error
    }
    delete[] valid;
    generateFeature();
    for (int i = 0; i < featSize; i++) {
        // print(weight[res][feat[i]]);
        score += weight[res][feat[i]];
    }
    return res;
}

State* State::transit(int action) {
    prevact = action;
    if (false) {
    } else if (action == LeftArc) {
        int s0i = stack.back();
        int n0i = buffer;
        stack.pop_back();
        edges[context(n0i,L,2)] = edges[context(n0i,L,1)];
        edges[context(s0i,H,1)] = n0i;
        edges[context(n0i,L,1)] = s0i;

    } else if (action == RightArc) {
        int n0i = buffer;
        int s0i = stack.back();
        buffer++;
        stack.push_back(n0i);
        edges[context(s0i,R,2)] = edges[context(s0i,R,1)];
        edges[context(n0i,H,2)] = edges[context(s0i,H,1)];
        edges[context(s0i,R,1)] = n0i;
        edges[context(n0i,H,1)] = s0i;

    } else if (action == Shift) {
        int n0i = buffer;
        buffer++;
        stack.push_back(n0i);

    } else if (action == Reduce) {
        stack.pop_back();
    } else {
    }
    isFinal = stack.size() == 1 && bufferIsEmpty();
    return this;
}

const int State::predAction() {
    generateFeature();

    bool* valid = validActions();
    double* scores = new double[nActions];

    for (int j = 0; j < nActions; j++) {
        if (!valid[j]) {
            scores[j] = doubleMin;
        } else {
            scores[j] = 0.0;
            for (int i = 0; i < featSize; i++) {
                scores[j] += weight[j][feat[i]];
            }
        }
    }
    int res;
    for (int i = 0; i < nActions; i++) {
        res = argmax(scores, nActions);
        if (valid[res]) {
            score += scores[res];
            delete[] valid, scores;
            return res;
        }
        scores[res] = doubleMin;
    }
    isFinal = true;
    return NOACTION;
}

Word& State::indToWord(int i) {
    if (0 <= i && i < sentSize) return (*sent)[i];
    else return (*sent)[0];
}

void State::generateFeature() {
    if (feat) return;
    int s0i = stack.empty() ? 0 : stack.back();
    int n0i = bufferIsEmpty() ? 0 : buffer;
    Word& s0  = indToWord(s0i);
    Word& n0  = indToWord(n0i);
    Word& n1  = indToWord(n0i+1);
    Word& n2  = indToWord(n0i+2);
    Word& s0h  = indToWord( edges[context(s0i,H,1)] );
    Word& s0l  = indToWord( edges[context(s0i,L,1)] );
    Word& s0r  = indToWord( edges[context(s0i,R,1)] );
    Word& n0l  = indToWord( edges[context(n0i,L,1)] );
    Word& s0h2 = indToWord( edges[context(s0i,H,2)] );
    Word& s0l2 = indToWord( edges[context(s0i,L,2)] );
    Word& s0r2 = indToWord( edges[context(s0i,R,2)] );
    Word& n0l2 = indToWord( edges[context(n0i,L,2)] );
    int s0vr = edges[context(s0i,R,2)] != -1 ? 2 : edges[context(s0i,R,1)] != -1 ? 1 : 0;
    int s0vl = edges[context(s0i,L,2)] != -1 ? 2 : edges[context(s0i,L,1)] != -1 ? 1 : 0;
    int n0vl = edges[context(n0i,L,2)] != -1 ? 2 : edges[context(n0i,L,1)] != -1 ? 1 : 0;
    int distance = s0i != 0 && n0i != 0 ? min(abs(s0i-n0i), 5) : 0;

    int i = 0;
    featSize = 57;
    feat = new int[featSize];
    // featSize = 1;
    feat[0]=tohash(dim, 0, s0.w);
    feat[1]=tohash(dim, 1, s0.p);
    feat[2]=tohash(dim, 2, s0.w, s0.p);
    feat[3]=tohash(dim, 3, n0.w);
    feat[4]=tohash(dim, 4, n0.p);
    feat[5]=tohash(dim, 5, n0.w, n0.p);
    feat[6]=tohash(dim, 6, n1.w);
    feat[7]=tohash(dim, 7, n1.p);
    feat[8]=tohash(dim, 8, n1.w, n1.p);
    feat[9]=tohash(dim, 9, n2.w);
    feat[10]=tohash(dim, 10, n2.p);
    feat[11]=tohash(dim, 11, n2.w, n2.p);
    feat[12]=tohash(dim, 12, s0.w, s0.p, n0.w, n0.p);
    feat[13]=tohash(dim, 13, s0.w, s0.p, n0.w);
    feat[14]=tohash(dim, 14, s0.w, n0.w, n0.p);
    feat[15]=tohash(dim, 15, s0.w, s0.p, n0.p);
    feat[16]=tohash(dim, 16, s0.p, n0.w, n0.p);
    feat[17]=tohash(dim, 17, s0.w, n0.w);
    feat[18]=tohash(dim, 18, s0.p, n0.p);
    feat[19]=tohash(dim, 19, n0.p, n1.p);
    feat[20]=tohash(dim, 20, n0.p, n1.p, n2.p);
    feat[21]=tohash(dim, 21, s0.p, n0.p, n1.p);
    feat[22]=tohash(dim, 22, s0h.p, s0.p, n0.p);
    feat[23]=tohash(dim, 23, s0.p, s0l.p, n0.p);
    feat[24]=tohash(dim, 24, s0.p, s0r.p, n0.p);
    feat[25]=tohash(dim, 25, s0.p, n0.p, n0l.p);
    feat[26]=tohash(dim, 26, s0.w, distance);
    feat[27]=tohash(dim, 27, s0.p, distance);
    feat[28]=tohash(dim, 28, n0.w, distance);
    feat[29]=tohash(dim, 29, n0.p, distance);
    feat[30]=tohash(dim, 30, s0.w, n0.w, distance);
    feat[31]=tohash(dim, 31, s0.p, n0.p, distance);
    feat[32]=tohash(dim, 32, s0.w, s0vr);
    feat[33]=tohash(dim, 33, s0.p, s0vr);
    feat[34]=tohash(dim, 34, s0.w, s0vl);
    feat[35]=tohash(dim, 35, s0.p, s0vl);
    feat[36]=tohash(dim, 36, n0.w, n0vl);
    feat[37]=tohash(dim, 37, n0.p, n0vl);
    feat[38]=tohash(dim, 38, s0h.w);
    feat[39]=tohash(dim, 39, s0h.p);
    feat[40]=tohash(dim, 40, s0l.w);
    feat[41]=tohash(dim, 41, s0l.p);
    feat[42]=tohash(dim, 42, s0r.w);
    feat[43]=tohash(dim, 43, s0r.p);
    feat[44]=tohash(dim, 44, n0l.p);
    feat[45]=tohash(dim, 45, s0h2.w);
    feat[46]=tohash(dim, 46, s0h2.p);
    feat[47]=tohash(dim, 47, s0l2.w);
    feat[48]=tohash(dim, 48, s0l2.p);
    feat[49]=tohash(dim, 49, s0r2.w);
    feat[50]=tohash(dim, 50, s0r2.p);
    feat[51]=tohash(dim, 51, n0l2.w);
    feat[52]=tohash(dim, 52, n0l2.p);
    feat[53]=tohash(dim, 53, s0.p, s0l.p, s0l2.p);
    feat[54]=tohash(dim, 54, s0.p, s0r.p, s0r2.p);
    feat[55]=tohash(dim, 55, s0.p, s0h.p, s0h2.p);
    feat[56]=tohash(dim, 56, n0.p, n0l.p, n0l2.p);


    // T(s0.w) T(s0.p) T(s0.w, s0.p) T(n0.w) T(n0.p) T(n0.w, n0.p)
    // T(n1.w) T(n1.p) T(n1.w, n1.p) T(n2.w) T(n2.p) T(n2.w, n2.p)
    //
    // //  from word pairs
    // T(s0.w, s0.p, n0.w, n0.p) T(s0.w, s0.p, n0.w) T(s0.w, n0.w, n0.p) T(s0.w, s0.p, n0.p)
    // T(s0.p, n0.w, n0.p) T(s0.w, n0.w) T(s0.p, n0.p) T(n0.p, n1.p)
    //
    // //  from three w
    // T(n0.p, n1.p, n2.p) T(s0.p, n0.p, n1.p) T(s0h.p, s0.p, n0.p) T(s0.p, s0l.p, n0.p)
    // T(s0.p, s0r.p, n0.p) T(s0.p, n0.p, n0l.p)
    //
    // // distance
    // T(s0.w, distance) T(s0.p, distance) T(n0.w, distance)
    // T(n0.p, distance) T(s0.w, n0.w, distance) T(s0.p, n0.p, distance)
    //
    // //  valency
    // T(s0.w, s0vr) T(s0.p, s0vr) T(s0.w, s0vl)
    // T(s0.p, s0vl) T(n0.w, n0vl) T(n0.p, n0vl)
    //
    // //  unigrams
    // T(s0h.w) T(s0h.p) T(s0l.w)
    // T(s0l.p) T(s0r.w) T(s0r.p) T(n0l.p)
    //
    // //  third-order
    // T(s0h2.w) T(s0h2.p) T(s0l2.w) T(s0l2.p) T(s0r2.w)
    // T(s0r2.p) T(n0l2.w) T(n0l2.p) T(s0.p, s0l.p, s0l2.p)
    // T(s0.p, s0r.p, s0r2.p) T(s0.p, s0h.p, s0h2.p) T(n0.p, n0l.p, n0l2.p)

}

int* State::heads() {
    int* res = new int[sentSize];
    for (int i = 0; i < sentSize; i++) {
        res[i] = edges[context(i,H,1)];
    }
    return res;
}

void State::toConll(ostream& os) {
    int* res = heads();
    for (int i = 1; i < sentSize; i++) {
        os << i << "\t" << (*sent)[i].toConll() << "\t" << res[i] << endl;
    }
    os << endl;
    delete[] res;
}

void State::toConll() {
    toConll(cout);
}

string State::to_string() {
    ostringstream osstack;
    osstack << step << " [ ";
    for (unsigned i = 0; i < stack.size(); i++) {
        Word& w = (*sent)[stack[i]];
        osstack << w.word << "/" << w.pos << " ";
    }
    osstack << "][ ";
    for (unsigned i = buffer; i < sentSize; i++) {
        Word& w = (*sent)[i];
        osstack << w.word << "/" << w.pos << " ";
    }
    osstack << "]";
    return osstack.str();
}

void initWeight() {
    weight = new double[nActions][dim];
    for (int i = 0; i < nActions; i++) {
        for (int j = 0; j < dim; j++) {
            weight[i][j] = 0.0;
        }
    }
}

State** State::toArray() {
    State** res = new State*[step+1];
    State* st = this;
    while (st->step > 0) {
        res[st->step] = st;
        st = st->prev;
    }
    res[st->step] = st;
    return res;
}

void maxViolate(State* pred, State* gold) {
    State** preds = pred->toArray();
    State** golds = gold->toArray();
    int K = pred->getStep() + 1;

    double* scoreDiff = new double[K];
    for (int i = 0; i < K; i++) {
        scoreDiff[i] = preds[i]->getScore() - golds[i]->getScore();
        // print(preds[i]->getScore());
        // print(golds[i]->getScore());
        // cout << i << " -> " << scoreDiff[i] << endl;
    }

    int action, **feat, fSize;
    int k = argmax(scoreDiff, K);
    for (int i = 1; i <= k; i++) {
        action = preds[i]->getPrevAct();
        feat   = preds[i]->getPrevState()->getFeat();
        fSize  = preds[i]->getPrevState()->getFeatSize();
        for (int j = 0; j < fSize; j++)
            weight[action][(*feat)[j]] -= 1.0;

        action = golds[i]->getPrevAct();
        feat   = golds[i]->getPrevState()->getFeat();
        fSize  = golds[i]->getPrevState()->getFeatSize();
        for (int j = 0; j < fSize; j++)
            weight[action][(*feat)[j]] += 1.0;
    }
    delete[] preds, golds, scoreDiff;
}

bool isIgnored(const string& word) {
    const int N = 6;
    static const string ignores[] = {"''", ",", ".", ":", "``", "''"};
    for (int i = 0; i < N; i++) {
        if (word == ignores[i])
            return true;
    }
    return false;
}

void evaluate(State* st) {
    int* pred; vector<Word>* gold; Word* w;
    pred = st->heads();
    gold = st->getSent();
    for (int i = 1; i < gold->size(); i++) {
        w = &(*gold)[i];
        if (!isIgnored(w->word)) {
            if (w->head == pred[i]) UASNum += 1;
            UASDen += 1;
        }
    }
}

int main() {
    const string file = "wsj_02-21.conll";
    const int ITERATION = 500;
    conll = readConll(file);
    initWeight();
    int nSamples = conll.size();
    // int nSamples = 1000;

    auto start = chrono::system_clock::now();
    int* idx = new int[nSamples];
    for (int i = 0; i < nSamples; i++) { idx[i] = i; }

    vector<Word>* sent;
    for (int i = 1; i < ITERATION; i++) {
        cout << "iteration: " << i << endl;
        shuffle<int>(idx, nSamples);
        for (int i = 0; i < nSamples; i++) {
            sent = &conll[idx[i]];
            int act;
            State pr(sent);
            State* pred = &pr;
            while (!pred->isFinal) {
                // cout << pred->to_string() << endl;
                act = pred->predAction();
                // cout << actionToString(act) << endl;
                if (act != NOACTION) {
                    pred = pred->nextState();
                    pred->transit(act);
                }
            }
            State go(sent);
            State* gold = &go;
            while (!gold->isFinal) {
                act = gold->goldAction();
                gold = gold->nextState();
                gold->transit(act);
            }
            evaluate(pred);
            maxViolate(pred, gold);
            delete pred, gold;
        }
        double res = (double)UASNum / (double)UASDen * 100;
        cout << "UAS: " << setprecision(5) << res << endl;
        UASDen = UASNum = 0;
    }
    auto end = chrono::system_clock::now();
    double elapsed = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    // cout << "total elapsed time: " << setprecision(5) << elapsed << " milliseconds" << endl;
    State st(&conll[2]);
    int act = st.goldAction();
    st.transit(Shift);
    // int** feat = st.getFeat();
    // Word& s0 = conll[0][0];
    // print(s0.w);
    // print(s0.word);
    // print(s0.p);
    // print(s0.pos);
    // for (int i = 0; i < 57; i++) {
    //     cout << (*feat)[i] << ", ";
    // }

    // cout << st.getStep() << " " << st.to_string() << endl;
    // st.toConll();
    // st.generateFeature();
    // int* f = st.getFeat();
    // for (int i = 0; i < 50; i++) {
    //     cout << f[i] << endl;
    // }
    delete[] weight, idx;
}

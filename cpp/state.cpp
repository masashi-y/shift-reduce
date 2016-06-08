#include "state.h"
// #define NODEBUG
#include "assert.h"
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <utility>

#define print(var) std::cout<<#var"= "<<var<<std::endl

const int dim = 1 << 25;
double (*weight)[dim];
vector<vector<Word> > trainsents;
vector<vector<Word> > testsents;
int UASDen = 0;
int UASNum = 0;
const int MAXFEATSIZE = 100;

State* State::stateBuffer[10000];
// State** State::stateBuffer = new State*[10000];
int State::nStateBuffer = 0;

State::State(vector<Word>* _sent):
    isFinal(false), step(0), score(0.0), stack(0), buffer(1)
{
    stack.push_back(0);
    sent = _sent;
    sentSize = sent->size();
    int N = INITEDGE();
    edges = new int[N];
    for (int i = 0; i < N; i++) { edges[i] = -1; }
    feat = nullptr;
    prev = nullptr;
    registerState();
}

State::~State() {
    delete[] edges;
    delete[] feat;
    // if (step > 1) delete prev; /////////////////
}

State::State(State& ro):
    isFinal(false), step(ro.step+1), score(ro.score), stack(ro.stack),
    buffer(ro.buffer), sent(ro.sent), prev(&ro), sentSize(ro.sentSize)
{
    int N = INITEDGE();
    edges = new int[N];
    memcpy(edges, ro.edges, sizeof(int)*N);
    feat = nullptr;
    registerState();
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

double State::goldAction(int* action) {
    bool* valid = validActions();
    int res = NOACTION;
    if (false) {
    }else if (valid[LeftArc] && wordAt(stack.back()).head == buffer) {
        res = LeftArc;
    }else if (valid[RightArc] && wordAt(buffer).head == stack.back()) {
        res = RightArc;
    }else if (valid[Reduce]) {
        if (bufferIsEmpty()) {
            res = Reduce;
        } else {
            // check if s0 has gold child in the buffer
            bool hasChild = false;
            for (int i = buffer; i < sentSize; i++) {
                if (wordAt(i).head == stack.back()) hasChild = true;
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
    double sc = 0.0;
    for (int i = 0; i < featSize; i++) {
        // print(weight[res][feat[i]]);
        sc += weight[res][feat[i]];
    }
    *action = res;
    return sc;
}

State* State::transit(int action, double sc) {
    prevact = action;
    score += sc;

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

double State::predAction(int* action) {
    generateFeature();

    bool* valid = validActions();
    bool actionExists = false;
    for (int i = 0; i < nActions; i++)
        actionExists = actionExists || valid[i];
    if (!actionExists) {
        delete[] valid;
        isFinal = true;
        return NOACTION;
    }

    double* scores = new double[nActions];
    for (int j = 0; j < nActions; j++) {
        if (!valid[j]) {
            scores[j] = numeric_limits<double>::lowest();
        } else {
            scores[j] = 0.0;
            for (int i = 0; i < featSize; i++) {
                scores[j] += weight[j][feat[i]];
            }
        }
    }
    int res = argmax(scores, nActions);
    double sc = scores[res];
    *action = res;
    delete[] valid;
    delete[] scores;
    return sc;
}

// const int State::predAction() {
//     generateFeature();
//
//     bool* valid = validActions();
//     double* scores = new double[nActions];
//
//     for (int j = 0; j < nActions; j++) {
//         if (!valid[j]) {
//             scores[j] = doubleMin;
//         } else {
//             scores[j] = 0.0;
//             for (int i = 0; i < featSize; i++) {
//                 scores[j] += weight[j][feat[i]];
//             }
//         }
//     }
//     int res;
//     for (int i = 0; i < nActions; i++) {
//         res = argmax(scores, nActions);
//         if (valid[res]) {
//             score += scores[res];
//             delete[] valid, scores;
//             return res;
//         }
//         scores[res] = doubleMin;
//     }
//     isFinal = true;
//     return NOACTION;
// }

Word& State::indToWord(int i) {
    if (0 <= i && i < sentSize) return wordAt(i);
    else return wordAt(0);
}

void State::generateFeature() {
    if (feat) return;
    int s0i = stack.empty() ? 0 : stack.back();
    int n0i = buffer;
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
    feat = new int[MAXFEATSIZE];

    T(s0.w) T(s0.p) T(s0.w, s0.p) T(n0.w) T(n0.p) T(n0.w, n0.p)
    T(n1.w) T(n1.p) T(n1.w, n1.p) T(n2.w) T(n2.p) T(n2.w, n2.p)
    //  from word pairs
    T(s0.w, s0.p, n0.w, n0.p) T(s0.w, s0.p, n0.w) T(s0.w, n0.w, n0.p)
    T(s0.w, s0.p, n0.p) T(s0.p, n0.w, n0.p) T(s0.w, n0.w) T(s0.p, n0.p) T(n0.p, n1.p)
    //  from three w
    T(n0.p, n1.p, n2.p) T(s0.p, n0.p, n1.p) T(s0h.p, s0.p, n0.p) T(s0.p, s0l.p, n0.p)
    T(s0.p, s0r.p, n0.p) T(s0.p, n0.p, n0l.p)
    // distance
    T(s0.w, distance) T(s0.p, distance) T(n0.w, distance)
    T(n0.p, distance) T(s0.w, n0.w, distance) T(s0.p, n0.p, distance)
    //  valency
    T(s0.w, s0vr) T(s0.p, s0vr) T(s0.w, s0vl)
    T(s0.p, s0vl) T(n0.w, n0vl) T(n0.p, n0vl)
    //  unigrams
    T(s0h.w) T(s0h.p) T(s0l.w)
    T(s0l.p) T(s0r.w) T(s0r.p) T(n0l.p)
    //  third-order
    T(s0h2.w) T(s0h2.p) T(s0l2.w) T(s0l2.p) T(s0r2.w)
    T(s0r2.p) T(n0l2.w) T(n0l2.p) T(s0.p, s0l.p, s0l2.p)
    T(s0.p, s0r.p, s0r2.p) T(s0.p, s0h.p, s0h2.p) T(n0.p, n0l.p, n0l2.p)

    featSize = i;
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
    for (int i = 1; i < sentSize; i++)
        os << i << "\t" << wordAt(i).toConll() << "\t" << res[i] << endl;
    os << endl;
    delete[] res;
}

void State::toConll() {
    toConll(cout);
}

string State::to_string() {
    ostringstream osstate;
    osstate << step << " [ ";
    for (unsigned i = 0; i < stack.size(); i++) {
        Word& w = wordAt(stack[i]);
        osstate << w.word << "/" << w.pos << " ";
    }
    osstate << "][ ";
    for (unsigned i = buffer; i < sentSize; i++) {
        Word& w = wordAt(i);
        osstate << w.word << "/" << w.pos << " ";
    }
    osstate << "]";
    return osstate.str();
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
    int K = min(pred->getStep(), gold->getStep()) + 1;

    double* scoreDiff = new double[K];
    for (int i = 0; i < K; i++)
        scoreDiff[i] = preds[i]->getScore() - golds[i]->getScore();

    int k = argmax(scoreDiff, K);
    int action, *feat, fSize;
    for (int i = 1; i <= k; i++) {
        action = preds[i]->getPrevAct();
        feat   = preds[i]->getPrevState()->getFeat();
        fSize  = preds[i]->getPrevState()->getFeatSize();
        for (int j = 0; j < fSize; j++)
            weight[action][feat[j]] -= 1.0;

        action = golds[i]->getPrevAct();
        feat   = golds[i]->getPrevState()->getFeat();
        fSize  = golds[i]->getPrevState()->getFeatSize();
        for (int j = 0; j < fSize; j++)
            weight[action][feat[j]] += 1.0;
    }
    delete[] preds;
    delete[] golds;
    delete[] scoreDiff;
}

bool isIgnored(const string& word) {
    static const int N = 6;
    static const string ignores[] = {"''", ",", ".", ":", "``", "''"};
    for (int i = 0; i < N; i++) {
        if (word == ignores[i]) return true;
    }
    return false;
}

void evaluate(State* st) {
    int* pred = st->heads();
    vector<Word>* gold = st->getSent();
    Word* w;
    for (int i = 1; i < gold->size(); i++) {
        w = &(*gold)[i];
        if (!isIgnored(w->word)) {
            if (w->head == pred[i]) UASNum += 1;
            UASDen += 1;
        }
    }
}

bool beamSort(State* lhs, State* rhs) {
    return lhs->getScore() > rhs->getScore(); 
}

vector<State*> State::expandPred() {
    if (isFinal) return vector<State*>(0);
    generateFeature();
    bool* valid = validActions();

    int nValids = 0;
    for (int i = 0; i < nActions; i++)
        nValids = nValids + (valid[i] ? 1 : 0);
    if (nValids == 0) {
        delete[] valid;
        isFinal = true;
        return vector<State*>(0);
    }

    vector<State*> res(0);
    State* st;
    double sc;
    for (int j = 0; j < nActions; j++) {
        if (valid[j]) {
            sc = 0.0;
            for (int i = 0; i < featSize; i++)
                sc += weight[j][feat[i]];
            st = nextState();
            st->transit(j, sc);
            res.push_back(st);
        }
    }
    delete[] valid;
    return res;
}

vector<State*> State::expandGold() {
    if (isFinal) return vector<State*>(0);
    int act;
    double sc = goldAction(&act);
    State* st = nextState();
    st->transit(act, sc);
    return vector<State*>{st};
}

State* beamSearchParser(vector<Word>& sent,
                        unsigned beamSize,
                        vector<State*> (State::*expand)()) {

    vector<vector<State*> > chart = { {new State(&sent)} };
    vector<State*> states, expnd; State* st;
    int i = 0;
    while (i < chart.size()) {
        states = chart[i];
        if (states.size() > beamSize)
            sort(states.begin(), states.end(), beamSort);
        for (int j = 0; j < min((unsigned)states.size(), beamSize); j++) {
            expnd = std::move ( (states[j]->*expand)() );
            for (int k = 0; k < expnd.size(); k++) {
                st = expnd[k];
                while (st->getStep() >= chart.size())
                    chart.push_back(vector<State*>(0));
                chart[st->getStep()].push_back(st);
            }
        }
        i++;
    }
    sort(states.begin(), states.end(), beamSort);
    return chart.back().back();

}

int main() {
    const string trainfile = "wsj_02-21.conll";
    const string testfile = "wsj_23.conll";
    const int ITERATION = 30;
    trainsents = readConll(trainfile);
    testsents  = readConll(testfile);
    initWeight();
    int nSamples = trainsents.size();
    int nTestSamples = testsents.size();
    // int nSamples = 1000;

    chrono::system_clock::time_point start, end, iter1, iter2;
    double res, elapsed;
    start = chrono::system_clock::now();
    iter1 = start;
    int* idx = new int[nSamples];
    for (int i = 0; i < nSamples; i++) { idx[i] = i; }

    vector<Word>* sent;
    State *pred, *gold;
    for (int i = 1; i < ITERATION; i++) {
        cout << "iteration: " << i << endl;
        shuffle<int>(idx, nSamples);
        for (int i = 0; i < nSamples; i++) {
            if (i % 100 == 0) cout << i << "\r" << flush;
            sent = &trainsents[idx[i]];
            gold = beamSearchParser(*sent, 1, &State::expandGold);
            pred = beamSearchParser(*sent, 10, &State::expandPred);
            evaluate(pred);
            maxViolate(pred, gold);
            if (i % 1 == 0) State::clearStates();
        }
        res = (double)UASNum / (double)UASDen * 100;
        cout << "TRAIN UAS: " << setprecision(5) << res << endl;
        UASDen = UASNum = 0;

        for (int i = 0; i < nTestSamples; i++) {
            sent = &testsents[i];
            pred = beamSearchParser(*sent, 10, &State::expandPred);
            evaluate(pred);
            if (i % 1 == 0) State::clearStates();
        }
        res = (double)UASNum / (double)UASDen * 100;
        cout << "TEST UAS: " << setprecision(5) << res << endl;
        UASDen = UASNum = 0;

        iter2 = chrono::system_clock::now();
        elapsed = chrono::duration_cast<chrono::seconds>(iter2-iter1).count();
        iter1 = iter2;
        cout << "TOOK " << setprecision(5) << elapsed
             << " SECONDS IN ITER " << i << endl;
    }
    end = chrono::system_clock::now();
    elapsed = chrono::duration_cast<chrono::seconds>(end-start).count();
    cout << "TOTAL ELAPSED TIME: " << setprecision(5) << elapsed << " SECONDS" << endl;
        //     int act;
        //     double score;
        //     State pr(sent);
        //     State* pred = &pr;
        //     while (!pred->isFinal) {
        //         score = pred->predAction(&act);
        //         if (act != NOACTION) {
        //             pred = pred->nextState();
        //             pred->transit(act, score);
        //         }
        //     }
        //     State go(sent);
        //     State* gold = &go;
        //     while (!gold->isFinal) {
        //         score = gold->goldAction(&act);
        //         gold = gold->nextState();
        //         gold->transit(act, score);
        //     }
        //     evaluate(pred);
        //     maxViolate(pred, gold);
        //     delete pred;
        //     delete gold;
        // }
}

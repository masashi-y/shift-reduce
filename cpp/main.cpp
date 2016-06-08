#include "state.h"

double (*weight)[dim];
vector<vector<Word> > trainsents;
vector<vector<Word> > testsents;
int UASDen = 0;
int UASNum = 0;

void initWeight() {
    weight = new double[nActions][dim];
    for (int i = 0; i < nActions; i++) {
        for (int j = 0; j < dim; j++) {
            weight[i][j] = 0.0;
        }
    }
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
    for (unsigned i = 1; i < gold->size(); i++) {
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

State* beamSearchParser(vector<Word>& sent,
                        unsigned beamSize,
                        vector<State*> (State::*expand)()) {

    vector<vector<State*> > chart = { {new State(&sent)} };
    vector<State*> states, expnd; State* st;
    unsigned i = 0;
    while (i < chart.size()) {
        states = chart[i];
        if (states.size() > beamSize)
            sort(states.begin(), states.end(), beamSort);
        for (unsigned j = 0; j < min((unsigned)states.size(), beamSize); j++) {
            expnd = std::move ( (states[j]->*expand)() );
            for (unsigned k = 0; k < expnd.size(); k++) {
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

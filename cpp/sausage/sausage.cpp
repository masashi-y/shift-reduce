#include "sausage.h"

using namespace wcn_parser;

std::unordered_map<std::string,int> words;
std::unordered_map<std::string,int> tags;
std::unordered_map<std::string,int> labels;


namespace wcn_parser {

int getOrAdd(std::unordered_map<std::string,int>& dict, const std::string& key) {
    int id = dict[key];
    if (id == 0) {
        id = dict.size() + 1;
        dict[key] = id;
    }
    return id;
}

Edge::Edge() 
    : word(getOrAdd(words, none)), tag(getOrAdd(tags, none)),
      head(-1), label(getOrAdd(labels, none)), wordstr(none),
      tagstr(none), labelstr(none), prob(0.0), isdisfl(false) {};

Edge::Edge(cstring& _word, cstring& _tag, cstring& _label, int _head, cstring& _status, double _prob, bool _isdisfl)
    : word(getOrAdd(words, _word)), tag(getOrAdd(tags, _tag)), head(_head),
      status(_status), label(getOrAdd(labels, _label)), wordstr(_word),
      tagstr(_tag), labelstr(_label), prob(_prob), isdisfl(_isdisfl) {};

std::ostream& operator<<(std::ostream& os, const Cell& cell) {
    bool best_path = true;
    for (unsigned i = 0; i < cell.size(); i++) {
        const Edge& edge = cell[i];
        os << cell.id << '\t'
           << edge.wordstr << '\t'
           << edge.status  << '\t'
           << (best_path ? cell.gold.wordstr : "-") << '\t'
           << edge.tagstr  << '\t'
           << (best_path ? cell.gold.tagstr : "-") << '\t'
           << edge.head    << '\t'
           << edge.labelstr << '\t'
           << (edge.isdisfl ? "DISFL" : "-") << '\t'
           << std::setprecision(7) << edge.prob << '\n';
        best_path = false;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Sausage& sau) {
    os << "# " << sau.id << std::endl;
    os << "# TRANSCRIPTION: " << sau.trans << std::endl;
    os << "# ASR ONE-BEST : " << sau.asr1b << std::endl;
    for (int i = 0; i < sau.size(); i++) {
        os << sau[i];
    }
    return os;
}

std::vector<Sausage> readConll(const std::string& file) {
    std::ifstream conllFile(file);
    std::string line, sw_id, trans, asr1b;
    std::string word, tag, label, status, gstatus;
    std::string gword, gtag, tmp;
    int head, id; double prob;
    std::vector<Sausage> res;
    std::vector<Cell> cells;
    std::vector<Edge> edges;
    bool disfl, best_path = true;
    int i = 1;
    while (getline(conllFile, line)) {
        if (line.size() <= 1) {
            cells.push_back(Cell(i, gstatus, edges, Edge(gword, gtag)));
            res.push_back(Sausage(sw_id, cells, trans, asr1b));
            edges.clear();
            cells.clear();
            best_path = true;
            i = 1;
        } else if (line.substr(0, 4) == "# sw") {
            sw_id = line.substr(2, line.size()-1);

        } else if (line.substr(0, 4) == "# TR") {
            trans = line.substr(17, line.size()-1);

        } else if (line.substr(0, 4) == "# AS") {
            asr1b = line.substr(17, line.size()-1);

        } else if (line.substr(0, 2) == "# ") {
            continue;
        } else {
            std::istringstream is(line);
            is >> id >> word;
            if (id != i) {
                cells.push_back(Cell(i, gstatus, edges, Edge(gword, gtag)));
                edges.clear();
                best_path = true;
                i++;
            }
            is >> status;
            if (best_path) gstatus = status;
            is >> (best_path ? gword : tmp);
            is >> tag;
            is >> (best_path ? gtag : tmp);

            is >> head >> label >> tmp;
            disfl = tmp == "DISFL";
            is >> prob;
            best_path = false;
            edges.push_back(Edge(word, tag, label, head, status, prob, disfl));
        }
    }
    return res;
}

} // namespace wcn_parser


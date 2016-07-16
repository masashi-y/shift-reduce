
#ifndef SHIFT_REDUCE_SAUSAGE_H_
#define SHIFT_REDUCE_SAUSAGE_H_

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <iomanip>

namespace wcn_parser
{

typedef const std::string cstring;

const std::string none = "" ;

class Edge;
class Cell;
class Sausage;

class Edge
{
public:
    Edge();
    Edge(cstring& _word, cstring& _tag, cstring& _label=none, int _head=-1,
            cstring& _status=none, double _prob=0.0, bool _isdisfl=false);
    friend Cell;

    const int word;
    const int tag;
    const int label;
    const int head;
    const std::string wordstr;
    const std::string tagstr;
    const std::string labelstr;
    const std::string status;
    double prob;
    bool isdisfl;
};

class Cell
{
public:
    Cell(int _id, cstring& _status,
            const std::vector<Edge>& _edges, const Edge& _gold)
        : id(_id), status(_status), edges(_edges), gold(_gold) {};

    const Edge& operator[](int i) const { return edges[i]; }
    unsigned size() const { return edges.size(); }
    friend std::ostream& operator<<(std::ostream& os, const Cell& cell);
private:
    const int id;
    const std::string status;
    const std::vector<Edge> edges;
    const Edge gold;
};

class Sausage
{
public:
    Sausage() {};
    Sausage(cstring& _id, const std::vector<Cell>& _cells,
        cstring& _trans, cstring& _asr1b)
    : id(_id), cells(_cells), trans(_trans), asr1b(_asr1b) {};

    friend std::ostream& operator<<(std::ostream& os, const Sausage& sau);

    const Cell& operator[](int i) const { return cells[i]; }
    unsigned size() const { return cells.size(); }
private:
    const std::string id;
    const std::vector<Cell> cells;
    const std::string trans;
    const std::string asr1b;
};

std::vector<Sausage> readConll(const std::string& file);

} // namespace wcn_parser

#endif // SHIFT_REDUCE_SAUSAGE_H_

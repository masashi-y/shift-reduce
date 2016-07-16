#include "sausage.h"

using namespace wcn_parser;

int main(int argc, char const* argv[])
{
    const std::string file = argv[1];
    std::vector<Sausage> saus = readConll(file);
    int cnt = 0;
    for (int i = 0; i < saus.size(); i++) {
        if (saus[i].size() > 0)
            cnt++;
    }
    std::cout << file << " contains " << cnt << " lattices." << std::endl;
}

#include "StringParsing.h"

using namespace std;

vector<string> split(string input, string delims)
{
    vector<string> res;
    int lastpos = 0;
    for (size_t i = 0; i < input.size(); i++)
    {
        for (size_t j = 0; j < delims.size(); j++)
        {
            if (input.at(i) == delims.at(j))
            {
                string sub = input.substr(lastpos, i - lastpos);
                if (sub.length() > 0)
                {
                    res.push_back(sub);
                }
                lastpos = i + 1;
            }
        }
    }
    string sub = input.substr(lastpos);
    if (sub.length() > 0)
    {
        res.push_back(sub);
    }
    return res;
}
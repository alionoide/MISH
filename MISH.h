#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include "StringParsing.h"

using namespace std;

struct Command
{
    string command;
    vector<string> args;
    string redirout;
    string redirin;
};

int main(int argc, char** argv);
void execute(vector<vector<Command>> commands);
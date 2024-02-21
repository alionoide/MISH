#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <sys/wait.h>
#include "StringParsing.h"
#include <readline/readline.h>
#include <readline/history.h>

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
void parse(char* inputChars);
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include "StringParsing.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <hash_map>

using namespace std;

struct Command
{
    string command;
    vector<string> args;
    string redirout;
    string redirin;
};

int main(int argc, char** argv);
int execute(vector<vector<Command>> commands, bool skipwait);
int parse(char* inputChars);
int runBuiltIn(Command command);
string createStartup();
void runFile(string filename);
void outputCommands(vector<vector<Command>> commands);
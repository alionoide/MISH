#include "mish.h"

// The debug flag will enable some debug output, useful to figure out what it thinks your command is
//#define DEBUG
// This will disable the use of the .mishrc file, useful if it is causing issues
//#define DISABLE_STARTUP 
#define STARTUP_FILE ".mishrc"

using namespace std;

// Dictionary to store aliases
unordered_map<string, vector<string>> aliasMap;

// Main, handles user input
int main(int argc, char** argv)
{
    if (argc == 1) // console read
    {
#ifndef DISABLE_STARTUP
        string startupPath = createStartup();
        runFile(startupPath);
#endif // !DISABLE_STARTUP

        rl_bind_key('\t', rl_complete);
        using_history();
        while (1)
        {
            char* path = nullptr;
            path = getcwd(path, 0);
            if (path == nullptr)
            {
                perror("Error: Current working directory");
                exit(0);
            }
            string prompt(path);
            free(path);

            prompt = "MISH:" + prompt + "$ ";

            char* input = readline(prompt.c_str());
            
            if (!input)
                break;

            add_history(input);

            parse(input);

            free(input);
        }
    }
    else // file read
    { 
        if (argc != 2)
        {
            cout << "Invalid commandline arguments" << endl << "Usage: mish [scriptFile]" << endl;
            exit(0);
        }

        runFile(argv[1]);
    }
    exit(0);
}

// This function will execute the command datastructure
int execute(vector<vector<Command>> commands, bool skipwait)
{
    for (size_t i = 0; i < commands.size(); i++)
    {
        int pipeRes[2] = { -1 };
        int prevPipe = -1;
        for (size_t j = 0; j < commands[i].size(); j++)
        {
            // built in commands
            if (commands[i][j].command == "cd" || commands[i][j].command == "exit" || commands[i][j].command == "alias" || (commands[i][j].args.size() > 1 && commands[i][j].args[1] == "="))
            {
                if (commands[i].size() > 1)
                {
                    cout << "Error: Cannot use pipe with built in command " << commands[i][j].command << endl;
                    return -1;
                }

                int res = runBuiltIn(commands[i][j]);
                if (res < 0)
                {
                    return -1;
                }
            }
            else
            {
                // pipe
                prevPipe = pipeRes[0];
                if (j < commands[i].size() - 1)
                {
                    int res = pipe(pipeRes);

                    if (res < 0)
                    {
                        cout << "Error in pipe: " << errno << endl;
                        return -1;
                    }
                }

                int pid = fork();

                // child
                if (pid == 0)
                {
                    // alias
                    if (aliasMap.contains(commands[i][j].command))
                    {
                        // copy args from alias
                        vector<string> newArgs(aliasMap.at(commands[i][j].command));
                        // add extra args
                        newArgs.insert(newArgs.end(), commands[i][j].args.begin() + 1, commands[i][j].args.end());
                        commands[i][j].args = newArgs;
                        commands[i][j].command = newArgs[0];
                    }

                    // convert args vector to char**
                    vector<const char*> interm;
                    for (size_t k = 0; k < commands[i][j].args.size(); k++)
                    {
                        interm.push_back(commands[i][j].args[k].c_str());
                    }
                    interm.push_back(nullptr);
                    char** argv = const_cast<char**>(&interm[0]);

                    // file redirects
                    if (!commands[i][j].redirin.empty())
                    {
                        int fd = open(commands[i][j].redirin.c_str(), O_RDONLY);
                        if (fd > 0)
                        {
                            dup2(fd, STDIN_FILENO);
                        }
                        else
                        {
                            perror("Error: Cannot open file");
                            cout << "File: " << commands[i][j].redirin << endl;
                            exit(0);
                        }
                    }
                    if (!commands[i][j].redirout.empty())
                    {
                        int fd = open(commands[i][j].redirout.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
                        if (fd > 0)
                        {
                            dup2(fd, STDOUT_FILENO);
                        }
                        else
                        {
                            perror("Error: Cannot open file");
                            cout << "File: " << commands[i][j].redirout << endl;
                            exit(0);
                        }
                    }

                    // pipe
                    if (j > 0)
                    {
#ifdef DEBUG
                        cout << "Setting redirect input to " << prevPipe << endl;
#endif // DEBUG
                        int res = dup2(prevPipe, STDIN_FILENO);
                        if (res < 0)
                        {
                            perror("Error: Failure in duplication of input pipe");
                            exit(0);
                        }
                    }
                    if (j < commands[i].size() - 1)
                    {
#ifdef DEBUG
                        cout << "Setting redirect output to " << pipeRes[1] << endl;
#endif // DEBUG
                        int res = dup2(pipeRes[1], STDOUT_FILENO);
                        if (res < 0)
                        {
                            perror("Error: Failure in duplication of output pipe");
                            exit(0);
                        }
                    }

                    // execute
                    int res = execvp(commands[i][j].command.c_str(), argv);
                    perror("Error: Failure on exec");
                    cout << "Command: " << commands[i][j].command << endl;
                    exit(0);
                }
                // error
                if (pid < 0)
                {
                    perror("Error: Failure on fork");
                    cout << "Command: " << commands[i][j].command << endl;
                    return -1;
                }
                // parent
                if (pid > 0)
                {
                    // close unneeded pipes
                    if (j < commands[i].size() - 1)
                    {
                        close(pipeRes[1]);
                    }
                    if (j > 0)
                    {
                        close(prevPipe);
                    }
                }
            }
        }
    }
    while (!skipwait && wait(nullptr) > 0)
    {
        // wait for children
    }
    return 0;
}

// This function parses the input character array into a vector<vector<Command>>
// the outer vector is a list of all the parrallel commands that need executed
// the inner layer are chains of commands that need piped together
int parse(char* inputChars)
{
    string whitespace = " \t\n";
    string inp(inputChars);
    vector<vector<Command>> commands;

    // split all of the parallel commands that need executed
    auto parrallelCommandsToExecute = split(inp, "&");
    for (size_t i = 0; i < parrallelCommandsToExecute.size(); i++)
    {
        vector<Command> npcommand;

        // check for trailing pipes
        if (split(parrallelCommandsToExecute.at(i), whitespace).back().ends_with('|'))
        {
            cout << "Error: Cannot end command with pipe: " << parrallelCommandsToExecute.at(i) << endl;
            return -1;
        }

        // now split all of the commands that need piped together
        auto pipeSeparated = split(parrallelCommandsToExecute.at(i), "|");

        for (size_t j = 0; j < pipeSeparated.size(); j++)
        {
            Command cmd;
            auto inputRedirSep = split(pipeSeparated.at(j), "<");
            auto outputRedirSep = split(pipeSeparated.at(j), ">");
            auto inputOutputRedirSep = split(pipeSeparated.at(j), "<>");

            // now create "tokens" based on whitespace
            cmd.args = split(inputOutputRedirSep.at(0), whitespace);

            // error checking
            if (cmd.args.size() == 0)
            {
                cout << "Error: Invalid format: One or more commands was whitespace: " << pipeSeparated.at(j) << endl;
                return -1;
            }
            cmd.command = cmd.args.at(0);

            if (cmd.command.size() == 0)
            {
                cout << "Error: Invalid format: One or more commands was whitespace: " << pipeSeparated.at(j) << endl;
                return -1;
            }
            else if (j > 0 && inputRedirSep.size() > 1)
            {
                cout << "Error: Invalid format: Cannot do both input redirection AND pipe redirection: " << pipeSeparated.at(j) << endl;
                return -1;
            }
            else if (j < pipeSeparated.size() - 1 && outputRedirSep.size() > 1)
            {
                cout << "Error: Invalid format: Cannot do both output redirection AND pipe redirection: " << pipeSeparated.at(j) << endl;
                return -1;
            }
            else if (inputRedirSep.size() > 2 || outputRedirSep.size() > 2)
            {
                cout << "Error: Invalid format: Too many input / output redirects: " << pipeSeparated.at(j) << endl;
                return -1;
            }
            else
            {
                // handle file redirects
                if (inputOutputRedirSep.size() == 1) // No redirects
                {
                }
                else if (inputOutputRedirSep.size() == 2) // Single input output redirects
                {
                    if (inputRedirSep.size() == 2)
                    {
                        auto inputRedir = split(inputRedirSep.at(1), whitespace);
                        if (inputRedir.size() != 1)
                        {
                            cout << "Error: Invalid format: Invalid input redirection: " << inputRedirSep.at(1) << endl;
                            return -1;
                        }
                        else
                        {
                            cmd.redirin = inputRedir.at(0);
                        }
                    }
                    else
                    {
                        auto outputRedir = split(outputRedirSep.at(1), whitespace);
                        if (outputRedir.size() != 1)
                        {
                            cout << "Error: Invalid format: Invalid output redirection: " << outputRedirSep.at(1) << endl;
                            return -1;
                        }
                        else
                        {
                            cmd.redirout = outputRedir.at(0);
                        }
                    }
                }
                else // multi input output redirects
                {
                    string input;
                    string output;
                    if (pipeSeparated.at(j).find('<') < pipeSeparated.at(j).find('>')) // input first
                    {
                        input = inputOutputRedirSep.at(1);
                        output = inputOutputRedirSep.at(2);
                    }
                    else // output first
                    {
                        input = inputOutputRedirSep.at(2);
                        output = inputOutputRedirSep.at(1);
                    }

                    auto inputRedir = split(input, whitespace);
                    auto outputRedir = split(output, whitespace);
                    if (inputRedir.size() != 1)
                    {
                        cout << "Error: Invalid format: Invalid input redirection: " << input << endl;
                        return -1;
                    }
                    else if (outputRedir.size() != 1)
                    {
                        cout << "Error: Invalid format: Invalid output redirection: " << output << endl;
                        return -1;
                    }
                    else
                    {
                        cmd.redirin = inputRedir.at(0);
                        cmd.redirout = outputRedir.at(0);
                    }
                }
                npcommand.push_back(cmd);
            }
        }

        commands.push_back(npcommand);
    }

#ifdef DEBUG
    outputCommands(commands);
    cout << "executing..." << endl << endl;
#endif // DEBUG
    return execute(commands, inp.ends_with('&'));
}

// debug function used to output the command datastructure
void outputCommands(vector<vector<Command>> commands)
{
    for (size_t i = 0; i < commands.size(); i++)
    {
        cout << "Command " << i << ": " << endl;
        for (size_t j = 0; j < commands[i].size(); j++)
        {
            cout << commands[i][j].command << endl << "Args: ";
            for (size_t k = 0; k < commands[i][j].args.size(); k++)
            {
                cout << commands[i][j].args[k] << ", ";
            }
            cout << endl;
            if (!commands[i][j].redirin.empty())
            {
                cout << "Redirected from: " << commands[i][j].redirin << endl;
            }
            if (!commands[i][j].redirout.empty())
            {
                cout << "Redirected to: " << commands[i][j].redirout << endl;
            }
            if (j != commands[i].size() - 1)
            {
                cout << "Piped to" << endl;
            }
        }
    }
}

// This function will run any of the built in commands
int runBuiltIn(Command cmd)
{
    // cd
    if (cmd.command == "cd") 
    {
        if (cmd.args.size() != 2)
        {
            cout << "Error: cd - invalid number of arguments" << endl;
            return -1;
        }
        else if (!cmd.redirin.empty() || !cmd.redirout.empty())
        {
            cout << "Error: cd - cannot do input / output redirection" << endl;
            return -1;
        }

        int res = chdir(cmd.args[1].c_str());
        if (res < 0)
        {
            perror("Error: Cannot change directory");
            return -1;
        }
    }
    // exit
    else if (cmd.command == "exit")
    {
        if (cmd.args.size() > 2)
        {
            cout << "Error: Exit: too many arguments" << endl;
            return -1;
        }
        else if (cmd.args.size() == 1)
        {
            exit(0);
        }
        try
        {
            exit(stoi(cmd.args[1]));
        }
        catch (const std::exception&)
        {
            cout << "Error: Exit: Requires numeric argument" << endl;
            exit(1);
        }
    }
    // alias
    else if (cmd.command == "alias")
    {

        if (cmd.args.size() < 3)
        {
            cout << "Error: alias - too few arguments" << endl;
            return -1;
        }
        else if (!cmd.redirin.empty() || !cmd.redirout.empty())
        {
            cout << "Error: alias - cannot do input / output redirection" << endl;
            return -1;
        }

        vector<string>::const_iterator first = cmd.args.begin() + 2;
        vector<string>::const_iterator last = cmd.args.end();
        vector<string> alias(first, last);

        aliasMap.insert_or_assign(cmd.args[1], alias);
    }
    // environment assignment
    else if (cmd.args[1] == "=")
    {
        if (cmd.args.size() != 3)
        {
            cout << "Error: environment assignment - invalid number of arguments" << endl;
            return -1;
        }
        else if (!cmd.redirin.empty() || !cmd.redirout.empty())
        {
            cout << "Error: environment assignment - cannot do input / output redirection" << endl;
            return -1;
        }

        int res = setenv(cmd.args[0].c_str(), cmd.args[2].c_str(), 1);
        if (res < 0)
        {
            perror("Error: Cannot set environment variable");
            return -1;
        }
    }
    return 0;
}

// Reads file line by line and executes it
void runFile(string filename)
{
    ifstream fin;

    fin.open(filename);

    if (!fin.is_open())
    {
        cout << "Error opening " << filename << endl;
        exit(0);
    }

    //
    int i = 1;
    char input[1000];
    while (fin.getline(input, 1000))
    {
        int res = parse(input);
        if (res < 0)
        {
            cout << "Error: In file: " << filename << endl;
            cout << "Line: " << i << endl;
            return;
        }
        i++;
    }

    fin.close();
}

// this function creates the startup file if it dosnt exist
// it returns the path to the file
string createStartup()
{
    // create path as home directory
    string path = getenv("HOME");
    path += "/";
    path += STARTUP_FILE;

    // check if file exists
    ifstream f(path);
    if (f.good())
    {
        return path;
    }

    // create it otherwise
    ofstream fout;
    fout.open(path);
    if (!fout.is_open())
    {
        cout << "Error: Cannot create " << STARTUP_FILE << endl;
        exit(0);
    }
    // add a sample command
    fout << "alias ll ls -la" << endl;
    fout.close();
    return path;
}

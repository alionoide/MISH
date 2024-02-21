#include "mish.h"

#define DEBUG
// Todo: 
// Input redirection
// Pipes
// Enviroment vars
// others

using namespace std;

int main(int argc, char** argv)
{
    if (argc == 1) // console read
    {
        rl_bind_key('\t', rl_complete);
        using_history();
        while (1)
        {
            char* input = readline("MISH-$ ");
            
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

        ifstream fin;

        fin.open(argv[1]);

        if (!fin.is_open())
        {
            cout << "Error opening " << argv[1] << endl;
            exit(0);
        }

        char input[256];

        while (fin.getline(input, 255))
        {
            parse(input);
        }
    }
    exit(0);
}

void execute(vector<vector<Command>> commands)
{
    int pipeRes[2];
    int oldPipe;
    for (size_t i = 0; i < commands.size(); i++)
    {
        for (size_t j = 0; j < commands[i].size(); j++)
        {
            // pipe
            oldPipe = pipeRes[0];
            if (j < commands[i].size() - 1)
            {
                int res = pipe(pipeRes);
            
                if (res < 0)
                {
                    cout << "Error in pipe: " << errno << endl;
                    return;
                }
            }

            int pid = fork();

            if (pid == 0)
            {
                vector<const char*> interm;

                for (size_t k = 0; k < commands[i][j].args.size(); k++)
                {
                    interm.push_back(commands[i][j].args[k].c_str());
                }
                interm.push_back(nullptr);
                char** argv = const_cast<char**>(&interm[0]);


                if (!commands[i][j].redirin.empty())
                {
                    int fd = open(commands[i][j].redirin.c_str(), O_RDONLY);
                    if (fd > 0)
                    {
                        dup2(fd, STDIN_FILENO);
                    }
                    else
                    {
                        cout << "Error: Cannot open file, code: " << errno << endl;
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
                        cout << "Error: Cannot open file, code: " << errno << endl;
                        cout << "File: " << commands[i][j].redirout << endl;
                        exit(0);
                    }
                }
                //pipe
                if (j > 0)
                {
                    dup2(oldPipe, STDIN_FILENO);
                }
                if (j < commands[i].size() - 1)
                {
                    dup2(pipeRes[1], STDOUT_FILENO);
                }

                int res = execvp(commands[i][j].command.c_str(), argv);
                cout << "Error: Failure on exec, code: " << errno << endl;
                cout << "Command: " << commands[i][j].command << endl;
                exit(0);

            }
            if (pid < 0)
            {
                cout << "Error: Failure on fork, code: " << errno << endl;
                cout << "Command: " << commands[i][j].command << endl;
                return;
            }
        }
    }
    while (wait(nullptr) > 0)
    {
        // wait for children
    }
}


void parse(char* inputChars)
{
    string whitespace = " \t\n";
    string inp(inputChars);
    vector<vector<Command>> commands;
    auto parrallelCommandsToExecute = split(inp, "&");
    for (size_t i = 0; i < parrallelCommandsToExecute.size(); i++)
    {
        vector<Command> npcommand;
        auto pipeSeparated = split(parrallelCommandsToExecute.at(i), "|");

        for (size_t j = 0; j < pipeSeparated.size(); j++)
        {
            Command cmd;
            auto inputRedirSep = split(pipeSeparated.at(j), "<");
            auto outputRedirSep = split(pipeSeparated.at(j), ">");
            auto inputOutputRedirSep = split(pipeSeparated.at(j), "<>");

            cmd.args = split(inputOutputRedirSep.at(0), whitespace);
            if (cmd.args.size() == 0)
            {
                cout << "Error: One or more commands was whitespace: " << pipeSeparated.at(j) << endl;
                return;
            }
            cmd.command = cmd.args.at(0);

            if (cmd.command.size() == 0)
            {
                cout << "Error: One or more commands was whitespace: " << pipeSeparated.at(j) << endl;
                return;
            }
            else if (j > 0 && inputRedirSep.size() > 1)
            {
                cout << "Error: Cannot do both input redirection AND pipe redirection: " << pipeSeparated.at(j) << endl;
                return;
            }
            else if (j < pipeSeparated.size() - 1 && outputRedirSep.size() > 1)
            {
                cout << "Error: Cannot do both output redirection AND pipe redirection: " << pipeSeparated.at(j) << endl;
                return;
            }
            else if (inputRedirSep.size() > 2 || outputRedirSep.size() > 2)
            {
                cout << "Error: Too many input / output redirects: " << pipeSeparated.at(j) << endl;
                return;
            }
            else
            {
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
                            cout << "Error: Invalid input redirection: " << inputRedirSep.at(1) << endl;
                            return;
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
                            cout << "Error: Invalid output redirection: " << outputRedirSep.at(1) << endl;
                            return;
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
                        cout << "Error: Invalid input redirection: " << input << endl;
                        return;
                    }
                    else if (outputRedir.size() != 1)
                    {
                        cout << "Error: Invalid output redirection: " << output << endl;
                        return;
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

    cout << "executing..." << endl << endl;
#endif // DEBUG
    execute(commands);
}
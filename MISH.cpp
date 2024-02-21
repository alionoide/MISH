#include "mish.h"

using namespace std;

int main(int argc, char** argv)
{
    char inputChars[256];
    string whitespace = " \t\n";

    cout << "MISH-$ ";
    while (cin.getline(inputChars, 255))
    {
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
                cmd.command = cmd.args.at(0);

                if (cmd.command.size() == 0)
                {
                    cout << "Error: One or more commands was whitespace: " << pipeSeparated.at(j);
                }
                else if (j > 0 && inputRedirSep.size() > 1)
                {
                    cout << "Error: Cannot do both input redirection AND pipe redirection: " << pipeSeparated.at(j) << endl;
                }
                else if (j < pipeSeparated.size() - 1 && outputRedirSep.size() > 1)
                {
                    cout << "Error: Cannot do both output redirection AND pipe redirection: " << pipeSeparated.at(j) << endl;
                }
                else if (inputRedirSep.size() > 2 || outputRedirSep.size() > 2)
                {
                    cout << "Error: Too many input / output redirects! - " << pipeSeparated.at(j) << endl;
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
                        }
                        else if (outputRedir.size() != 1)
                        {
                            cout << "Error: Invalid output redirection: " << output << endl;
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
                if (j != commands[i].size()-1)
                {
                    cout << "Piped to" << endl;
                }
            }
        }

        cout << "executing..." << endl << endl;
        execute(commands);

        cout << "MISH-$ ";
    }

}

void execute(vector<vector<Command>> commands)
{
    for (size_t i = 0; i < commands.size(); i++)
    {
        int pid = fork();


        if (pid == 0)
        {
            vector<const char*> interm;

            for (size_t j = 0; j < commands[i][0].args.size(); j++)
            {
                interm.push_back(commands[i][0].args[j].c_str());
            }
            interm.push_back(nullptr);
            char** argv = const_cast<char**>(&interm[0]);

            int res = execvp(commands[i][0].command.c_str(), argv);
            cout << "Error in exec, code: " << res << endl;

            if (!commands[i][0].redirin.empty())
            {
                cout << "Redirects unsupported currently: " << commands[i][0].redirin << endl;
            }
            if (!commands[i][0].redirout.empty())
            {
                cout << "Redirects unsupported currently: " << commands[i][0].redirout << endl;
            }
            if (commands[i].size() > 1)
            {
                cout << "Pipes unsupported currently" << endl;
            }
        }
        if (pid < 0)
        {
            cout << "Error: Failure in fork on command: " << commands[i][0].command << endl;
        }
    }
    while (wait(nullptr) > 0)
    {
        // wait for children
    }
}
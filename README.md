# MISH

## Compiling 

Note: Uses [GNU Readline Library](https://tiswww.case.edu/php/chet/readline/readline.html)

`cmake CMakeLists.txt`

`cmake --build .`


## Running 

`MISH [scriptFile]`

## Additional features

- Command line completion
- Command line history
- Working directory in prompt
- Startup file ".mishrd" (in home directory)
- Alias (ex: `alias ll ls -la`)

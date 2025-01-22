## How to use?

**Step 1.** Compiling the program.
```cmd 
mkdir build
cd build
cmake ..
make 
cd ..
```

**Step 2.** Parsing the program by its name.
In the following command, the parser will parse ./tests/01.c
```cmd
./build/absint tests/easy1.c
```

## Point one and two
The code for the first two points, providing an abstract interpreter without the support for the fixpoint iteration nor code location is available in this same repo under the `atomic_commands` branch 

## Point three and four
The final version of the project, implementing fixpoints, code locations, while loop and widening, is available in this repo under the `master` branch.

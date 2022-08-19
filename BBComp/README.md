To run a function from `functions.json`:
1. Load the code string of the function from the json file. Remember to add your c/c++ file to the `Makefile`
2. Parse the code string with the function `parse()`, which returns an `ExpressionPtr pointer`
3. Create the input vector (look at the file `vector.h` for the constructors)
4. Compute the function with `evaluate(ExpressionPtr pointer, Vector input)`, which returns a double value

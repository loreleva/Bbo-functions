# Introduction
This python module provides functions that let one to search among the list of the [Surjanovic and Bingham Optimization Test Problems](https://www.sfu.ca/~ssurjano/optimization.html) and also implements the objective function's class, which returns an `objective_function` object needed in order to evaluate the objective function.
`example.py` shows an example of usage of the module.

# Module function description

## List of functions
- [get_json_functions()](#get_json_function)
- [search_functions()](#search_functions)

## List of variables
- [json_filepath](#json_filepath)

## List of classes
- [objective_function](#objective_function)

## Functions

<a name="get_json_function"></a>
### `get_json_functions(name: [str])`
Loads the json object of the functions and returns it. If the `name` string is defined, it returns the dictionary of data associated to the function `name`.

<a name="search_functions"></a>
### `search_functions(filters=None: [dict])`
Returns a list of the names of the functions.  
If `filters` is provided, it returns the names of functions that satisfy the filters. `filters` is a dictionary with the information fields of the functions as key (e.g., `filters = { "dimension" : 2, "minimum_f" = True }`).

## Variables

<a name="json_filepath"></a>
### `json_filepath: [str]`
Filepath of the json file of the functions data.

## Classes

<a name="objective_function"></a>
### `objective_function(name: [str], dim=None: [int], param=None: [dict])`
Objective function class which takes in input:
- The `name` of the objective function (this must be equal to the one in the json file)
- The dimension `dim` of the function's input
- A dictionary of parameters `param`, where the key is the parameter name and the value its value

### Attributes
- `name: [str]` Name of the objective function
- `dimension: [int]` Dimension of the function input
- `has_input_domain_range: [bool]` True when the range of the input domain is available
- `input_lb: [list]` Each index of the list represents a dimension, a value at that index represents the lower bound of the range of that dimension
- `input_ub: [list]` Each index of the list represents a dimension, a value at that index represents the upper bound of the range of that dimension
-	`input_opt: [int, float, list]` Function's input value of the global optimum. When the value is list of lists, it represents the multiple possible input values for the global optimum
-	`has_parameters: [bool]` It is True when the function accepts parameters
-	`parameters_description: [str]` Description of the function's parameters
-	`parameters: [dict]` Dictionary of parameters' values of the objective function
-	`parameters_names: [list]` List of the parameters names
-	`param_opt: [tuple]` Tuple with parameter name at idx 0, and dict of values for which opt is defined at idx 1
-	`opt: [float]` Function's global optimum
-	`implementation: [function]` implementation of the objective function

### Methods
- `update_parameters(parameters: [dict])` Updates the function's parameters with the values in `parameters`
- `evaluate(inp: [float, list])` Evalute the objective function on input `inp`

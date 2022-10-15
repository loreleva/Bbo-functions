# Table of contens
[Introduction](#introduction)  
[Surjanovic and Bingham Optimization Test Problems](#surjanovic-and-bingham-optimization-test-problems)  
[Black Box Optimization Competition](#black-box-optimization-competition)  

# Introduction
This repo contains a collection of functions useful to perform blackbox optimization.  
**Currently** the functions are taken from the following resources:
- [Surjanovic and Bingham Optimization Test Problems](https://www.sfu.ca/~ssurjano/optimization.html)
- [Black Box Optimization Competition](https://www.ini.rub.de/PEOPLE/glasmtbl/projects/bbcomp/)

# Surjanovic and Bingham Optimization Test Problems
Their project [site](https://www.sfu.ca/~ssurjano/optimization.html) contains multiple functions to test optimization algorithms.  
The `sfu/functions` folder groups the functions according to whether they accept parameters or not.  
The `sfu/functions/functions.json` file contains the description of the functions, each function is described as follows:

| Field			| Value type												| Description  |
| ---			| ---														| ---		   |
| `dimension`	| integer, string											| Dimension of the input of the function (`d` stands for any dimension size) |
| `minimum_x`	| float, list of list of float values, string, none			| Functions's input values where the function value is the global minimum.<br />If the value is a float number and dimension is greater than 1, that value is the same for all the dimensions.<br />If there is a list of lists, then each inner list contains valid input values for the global minimum, the content of an inner list is either float values or string, which contains the python code to compute the values    |
| `minimum_f`	| float, string, none										| Function's global minimum value.<br />If the value is a string, it contains the python code to compute the optimal function value  |
| `parameters`	| bool, string												| Description of the parameters of the function if it accepts them, otherwise the value is `false` |
| `input_domain`| list of list of float, list of list of string				| Range of the input domain. Each inner list represents the range of values for a specific dimension of the function, where usually the function is evaluated.<br />If the dimension is greater than 1 and there is only one inner list, that range is for all the dimensions of the function.<br />If an inner list contains a string, it contains the python code to compute the range |
| `filepath_r`	| string													| Filepath to the R implementation of the function |
| `filepath_m`	| string													| Filepath to the MATLAB implementation of the function |

# Black Box Optimization Competition
Collection of functions accumulated over five years of the competition. The functions are written in c++ and are computable only by their released software.  
More info in the README file in `BBComp`.

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
Their project site contains multiple functions to test optimization algorithms.  
The `sfu_MATLAB` folder group the functions according to whether they accept parameters or not.  
The `sfu_MATLAB/functions.json` file contains all the function names, along with their:
- Dimension (d stands for any dimension size)
- Filename of the MATLAB implementation

# Black Box Optimization Competition
Collection of functions accumulated over five years of the competition. The functions are written in c++ and are computable only by their released software.  
More info in the README file in `BBComp`.

import sys
from functions import *

if __name__ == "__main__":
	load_json("../functions/functions.json")
	select_function("ackley_function")
	print(dimension())
	print(evaluate(1))
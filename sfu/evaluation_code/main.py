import sys
import functions

if __name__ == "__main__":
	functions.load_json("../functions/functions.json")
	functions.select_function("ackley_function")
	print(functions.dimension())
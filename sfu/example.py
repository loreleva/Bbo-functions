from objective_function_class import *

if __name__ == "__main__":
	function_selected = "michalewicz_function"
	dim = 4
	print(search_functions(filters={"dimension":"d", "minimum_f":True}))
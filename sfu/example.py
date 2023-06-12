from objective_function_class import *

if __name__ == "__main__":
	function_selected = "michalewicz_function"
	dim = 4
	function_obj = objective_function(function_selected, dim=dim)
	print(function_obj.evaluate([4]*dim))
from objective_function_class import *

if __name__ == "__main__":
	function_selected = "rastrigin_function"
	dim = 10
	obj = objective_function(function_selected, dim)
	#print(obj.input_opt)
	print(obj.evaluate(obj.input_opt))
	print(obj.opt)
from objective_function_class import *

if __name__ == "__main__":
	function_selected = "shekel_function"
	dim = 4
	obj = objective_function(function_selected, dim)
	#print(obj.input_opt)
	#print(obj.evaluate(obj.input_opt))
	print(obj.opt)
	print(obj.evaluate(obj.input_opt))
import json, math

json_functions = None
function_name = None
function_informations = None
d = None

class JsonNotLoaded(Exception):
	pass

class FunctionNotSelected(Exception):
	pass

class FunctionDoesNotExists(Exception):
	pass

class FunctionNeedsDimension(Exception):
	pass

def load_json(filepath):
	global json_functions
	f = open(filepath)
	json_functions = json.load(f)
	f.close()
	return json_functions

def select_function(name):
	if not name in json_functions.keys():
		raise FunctionDoesNotExists("The function selected does not exists")
	
	global function_name, function_informations
	function_name = name
	function_informations = json_functions[name]

def functions_names_list():
	global json_functions
	if not json_functions:
		raise JsonNotLoaded("Json file not loaded")
	
	return [x for x in json_functions]

def dimension():
	global function_name, function_informations
	if not function_name:
		raise FunctionNotSelected("No function has been selected")
	
	try:
		dimension = int(function_informations["dimension"])
		return dimension
	except ValueError:
		return "d"

def minimum_point(dimension=None):
	# return the point coordinates of the minimum value of the function
	global function_name, function_informations
	if not function_name:
		raise FunctionNotSelected("No function has been selected")

	if type(function_informations["minimum_x"]) == str:
		local_var = {}
		if function_dimension() != "d":
			exec(function_informations["minimum_x"], globals(), local_var)
			return local_var["minimum_x"]
		else:
			if not dimension:
				raise FunctionNeedsDimension("The function needs the dimension value")
			
			global d
			d = dimension
			exec(function_informations["minimum_x"], globals(), local_var)
			return local_var["minimum_x"]
	elif type(function_informations["minimum_x"]) == list:
		return function_informations["minimum_x"]
	elif function_informations["minimum_x"] == None:
		return None
	else:
		return [function_informations["minimum_x"] for x in range(dimension)]

def minimum_value(dimension=None):
	# return the global minimum of the function
	global function_name, function_informations
	if not function_name:
		raise FunctionNotSelected("No function has been selected")

	if type(function_informations["minimum_f"]) == str:
		local_var = {}
		if function_dimension() != "d":
			exec(function_informations["minimum_f"], globals(), local_var)
			return local_var["minimum_f"]
		else:
			if not dimension:
				raise FunctionNeedsDimension("The function needs the dimension value")

			global d
			d = dimension
			exec(function_informations["minimum_f"], globals(), local_var)
			return local_var["minimum_f"]
	elif type(function_informations["minimum_f"]) == list:
		return function_informations["minimum_f"]
	elif function_informations["minimum_f"] == None:
		return None
	else:
		return function_informations["minimum_f"]

def parameters():
	global function_name, function_informations
	if not function_name:
		raise FunctionNotSelected("No function has been selected")

	if not function_informations["parameters"][0]:
		return "The function does not have parameters"
	else:
		return function_informations["parameters"][1]

def input_domain():
	pass

def evaluate_R():
	pass

def evaluate_m():
	pass




load_json("../functions/functions.json")
select_function("cross-in-tray_function")
print(dimension())
print(minimum_point(20))
print(minimum_value(20))
print(parameters())
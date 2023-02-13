import json



def print_json(path):

	with open(path, 'r') as json_file:
		json_object = json.load(json_file)

	# ~ print(json.dumps(json_object))
	print(json.dumps(json_object, indent=2))



def merge_json(files, out):

	info = []

	for f in files:

		with open(f, "r") as fi:
			temp = fi.read()
			info.append(temp.strip())


	print(temp)

	temp = "["
	temp += ",".join(info)
	temp += "]"

	with open(out, "w") as fi:

		fi.write(temp)




if __name__ == "__main__":


	# ~ files = ["food.json", "nutr.json", "amt.json"]

	# ~ for f in files:
		# ~ print_json(f)


	# ~ print_json("food.json")
	# ~ print_json("nutr.json")
	# ~ print_json("amt.json")

	# ~ print_json("diet_cols.json")
	print_json("diet_cols_concat.json")






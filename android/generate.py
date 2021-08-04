import os
import sys

sys.path.insert(0, '../godot-cpp')
import binding_generator

json_api_file = os.path.join(os.getcwd(), 'godot-headers', 'api.json')
binding_generator.generate_bindings(json_api_file, True)

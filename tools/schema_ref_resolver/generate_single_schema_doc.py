import json
from ref_resolver import RefResolver
import base64

f = open("glTF.schema.json")
j = json.loads(f.read())

# call to API resolve method
RefResolver("glTF.schema.json").resolve(j)

j_str = json.dumps(j, indent=2)

# Run json.dumps twice to get escaped string
print("const char *kglTFSchemaString = " + json.dumps(j_str) + ";\n")

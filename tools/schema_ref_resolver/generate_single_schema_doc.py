import json
from ref_resolver import RefResolver

f = open("glTF.schema.json")
j = json.loads(f.read())

# call to API resolve method
RefResolver("glTF.schema.json").resolve(j)

outfile = open("glTF.schema.resolved.json")
outfile.write(json.dumps(j, indent=2))

# for print with `end` parameter
from __future__ import print_function

import json
from ref_resolver import RefResolver
import base64

f = open("glTF.schema.json")
j = json.loads(f.read())

# call to API resolve method
RefResolver("glTF.schema.json").resolve(j)

j_str = json.dumps(j, indent=2)


# Run json.dumps twice to get escaped string
escaped_str = json.dumps(j_str)

# MSVC does not accept string larger than 16K.
# https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2026?view=vs-2019
# Also, it has a hard limit of 65,535 bytes even splitting a string with double quotation.
# So, we write string as an array of string(then application must concatenate it)

# https://stackoverflow.com/questions/9475241/split-string-every-nth-character
n = 8000 # Conservative number

splitted_string = [escaped_str[i:i+n] for i in range(0, len(escaped_str), n)]
#print(len(splitted_string))

print("const char *kglTFSchemaStrings[] = {", end='')
for (i, s) in enumerate(splitted_string):
    print(s, end='')
    if i != (len(splitted_string) - 1):
        print('",\n"', end='')

print("};")

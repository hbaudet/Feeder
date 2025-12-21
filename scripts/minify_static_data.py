Import("env")

import os
import json
import itertools
import string
import re


DATA_SRC = 'data_raw'
DATA_DST = 'data'

LETTERS = string.ascii_lowercase
NAMES = [''.join(p) for n in range(1, 3) for p in itertools.product(LETTERS, repeat=n)]
NAMES_SHORTENED = {}
LAST_NAME = 0

def minify_data(source, target, env):
    global LAST_NAME
    for filename in os.listdir(DATA_SRC):
        ## JSON FILES
        if filename.endswith(".json"):
            path = os.path.join(DATA_SRC, filename)
            with open(path, "r") as f:
                obj = json.load(f)
                ## Shorten Names to loos a few kB
                # for source in obj["ioMatrix"]:
                #     shortened_names = []
                #     for output in obj["ioMatrix"][source]:
                #         NAMES_SHORTENED[output] = NAMES[LAST_NAME]
                #         shortened_names.append(NAMES[LAST_NAME])
                #         LAST_NAME += 1
                #     obj["ioMatrix"][source] = shortened_names
                # for output in obj["outputs"]:
                #     output["name"] = NAMES_SHORTENED[output["name"]]
            path = os.path.join(DATA_DST, filename)
            with open(path, "w") as f:
                json.dump(obj, f, separators=(",", ":"))
        ## HTML FILES
        elif filename.endswith(".html"):
            path = os.path.join(DATA_SRC, filename)
            with open(path, "r", encoding="utf-8") as f:
                html = f.read()
                html = re.sub(r"<!--.*?-->", "", html, flags=re.DOTALL)  # remove comments
                html = re.sub(r">\s+<", "><", html)  # remove spaces between tags
                html = re.sub(r"\s{2,}", " ", html)  # collapse multiple spaces
            path = os.path.join(DATA_DST, filename)
            with open(path, "w") as f:
                f.write(html.strip())
        ## else copy the file with no change

env.AddPreAction("buildfs", minify_data)

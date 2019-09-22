#!/usr/bin/env python
import os
import re

project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

with open(os.path.join(project_root, "ctools", "__init__.pyi")) as f:
    s = f.read()

doc = re.split("##----*start----*##", s, 1, re.MULTILINE)[1].lstrip()

api_list = [i for i in re.split("#- api", doc) if i]

des_regex = re.compile("#- description:(?P<des>.*)")

lines = []

for api in api_list:
    match = des_regex.search(api)
    if not match:
        lines.append("```python\n{}\n```".format(api.strip()))
        lines.append("\n")
    else:
        description = match.group("des").strip()
        if description:
            lines.append("* {}\n".format(description))
        api = des_regex.sub("", api).strip()
        lines.append("```python\n{}\n```".format(api))
        lines.append("\n\n\n")


with open(os.path.join(project_root, "docs", "api.md"), "wt") as f:
    f.writelines(lines)

#!/usr/bin/env python
import io
import os
import sys

LICENSE = """
Copyright (c) 2019 ko han

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
""".lstrip()


def license_with_comment(prefix, suffix):
    file = io.StringIO()
    file.write(prefix)
    file.write("\n")
    file.write(LICENSE)
    file.write(suffix)
    file.write("\n")
    file.seek(0)
    return file.read()


py_license = license_with_comment('"""', '"""')
c_license = license_with_comment("/*", "*/")


def remove_old_license(prefix, suffix, data):
    data = data.lstrip()
    if not data.startswith(prefix):
        return data
    data = data.lstrip(prefix)
    lines = data.splitlines()
    idx = -1
    for i, line in enumerate(lines):
        if line.rstrip().endswith(suffix):
            idx = i
            break
    idx += 1
    if idx == len(lines):
        return ""

    return "\n".join(lines[idx:])


def handle_one_file(*filename):
    path = os.path.realpath(os.path.join(*filename))
    if path.endswith(".py") or path.endswith(".pyi"):
        prefix = suffix = '"""'
        li = py_license
    elif path.endswith(".c") or path.endswith(".h"):
        prefix = "/*"
        suffix = "*/"
        li = c_license
    else:
        return

    with open(path, "r") as f:
        data = f.read()

    if not data.startswith(li):
        with open(path, "w") as f:
            f.write(li)
            f.write(remove_old_license(prefix, suffix, data))


def main():
    for filename in sys.argv[1:]:
        if os.path.isdir(filename):
            for sub in os.listdir(filename):
                handle_one_file(filename, sub)
        else:
            handle_one_file(filename)


if __name__ == "__main__":
    main()

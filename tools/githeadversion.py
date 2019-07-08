#!/usr/bin/env python
import os
import subprocess
import re

package_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

os.chdir(package_root)


def git_version():
    def _minimal_ext_cmd(cmd):
        # construct minimal environment
        env = {}
        for k in ["SYSTEMROOT", "PATH", "HOME"]:
            v = os.environ.get(k)
            if v is not None:
                env[k] = v
        # LANGUAGE is used on win32
        env["LANGUAGE"] = "C"
        env["LANG"] = "C"
        env["LC_ALL"] = "C"
        out = subprocess.check_output(cmd, stderr=subprocess.STDOUT, env=env)
        return out

    try:
        out = _minimal_ext_cmd(["git", "rev-parse", "HEAD"])
        return out.strip().decode("ascii")
    except (subprocess.SubprocessError, OSError):
        return "Unknown"


def write_version():
    with open(os.path.join(package_root, "ctools", "version.py"), "rt") as f:
        value = f.read()

    git_v = git_version()
    version_regex = re.compile(r"^_version\s*=\s*(\"|\')(?P<version>.+)(\"|\')\s*$")
    new_lines = []
    for line in value.splitlines():
        match = version_regex.match(line)
        if match:
            v = match.group("version")
            line = line.replace(v, v + "+" + git_v[:7])
            new_lines.append(line)
        else:
            new_lines.append(line)

    value = "\n".join(new_lines) + "\n"
    with open(os.path.join(package_root, "ctools", "version.py"), "wt") as f:
        f.write(value)


if __name__ == "__main__":
    write_version()

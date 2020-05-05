# -*- coding: utf-8 -*-

# Copyright 2019 ko-han. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import re
import io
import os
from setuptools import setup, Extension, find_packages
from distutils.command.build_ext import build_ext
from distutils.command.build_clib import build_clib
from distutils import sysconfig


def _is_using_gcc(obj):
    is_gcc = False
    if obj.compiler.compiler_type == "unix":
        cc = sysconfig.get_config_var("CC")
        if not cc:
            cc = ""
        compiler_name = os.path.basename(cc)
        is_gcc = "gcc" in compiler_name
    return is_gcc


class new_build_clib(build_clib):
    def build_a_library(self, build_info, lib_name, libraries):
        if _is_using_gcc(self):
            args = build_info.get("extra_compiler_args") or []
            args.append("-std=c99")
            build_info["extra_compiler_args"] = args
        build_clib.build_a_library(self, build_info, lib_name, libraries)


class new_build_ext(build_ext):
    def build_extension(self, ext):
        if _is_using_gcc(self):
            if "-std=c99" not in ext.extra_compile_args:
                ext.extra_compile_args.append("-std=c99")
        build_ext.build_extension(self, ext)


here = os.path.abspath(os.path.dirname(__file__))


def source(*args):
    return [os.path.join("src", arg) for arg in args]


DEBUG = os.getenv("CTOOLS_DEBUG", "").upper() == "TRUE"


extra_extension_args = {}


if DEBUG:
    extra_extension_args.update(undef_macros=["NDEBUG"])
    print("-" * 80)
    print("Ctools enable DEBUG")
    print("-" * 80)
else:
    extra_extension_args.update(define_macros=[("NDEBUG", "1")])


def find_version():
    with open(os.path.join(here, "ctools", "__init__.py"), "rt") as f:
        value = f.read()

    regex = r"""__version__\s?=\s?['"](?P<version>.+)['"]"""
    match = re.search(regex, value)
    if match is None:
        raise RuntimeError("can't get version info")
    return match.group("version").strip()


extensions = [
    Extension(
        "ctools._ctools",
        source(
            "cachemap.c",
            "channel.c",
            "ttlcache.c",
            "functions.c",
            "module.c",
            "rbtree.c",
        ),
        language="c",
        **extra_extension_args
    ),
]

with io.open("README.rst", "rt", encoding="utf8") as f:
    readme = f.read()

setup(
    name="ctools",
    version=find_version(),
    author="hanke",
    author_email="hanke0@outlook.com",
    description="A collection of useful extensions for python implement in C.",
    url="https://github.com/ko-han/python-ctools",
    project_urls={
        "Bug Tracker": "https://github.com/ko-han/python-ctools/issues",
        "Documentation": "https://python-ctools.readthedocs.io",
        "Source Code": "https://github.com/ko-han/python-ctools",
    },
    license="Apache License 2.0",
    long_description=readme,
    long_description_content_type="text/x-rst",
    python_requires=">=3.5",
    include_package_data=True,
    zip_safe=False,
    ext_modules=extensions,
    packages=find_packages(include=["ctools", "ctools.*"]),
    classifiers=[
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: C",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: Implementation :: CPython",
        "Operating System :: OS Independent",
        "Operating System :: MacOS",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: POSIX",
        "Operating System :: Unix",
        "Topic :: Utilities",
        "Topic :: Software Development",
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
    ],
    cmdclass={"build_ext": new_build_ext, "build_clib": new_build_clib},
)

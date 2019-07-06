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

import io
import os
from setuptools import setup, Extension, find_packages


here = os.path.abspath(os.path.dirname(__file__))


def source(*args):
    return [os.path.join("src", *args)]


if os.getenv("CTOOLS_DEBUG", "").upper() == "ON":
    print("---------------CTOOLS DEBUG ON------------------")
    extra_extension_args = dict(undef_macros=["NDEBUG"])
else:
    print("---------------CTOOLS DEBUG OFF------------------")
    extra_extension_args = dict(define_macros=[("NDEBUG", "1")])


def find_version():
    d = {}
    with open(os.path.join(here, "ctools", "version.py"), "rt") as f:
        value = f.read()
        exec(value, d, d)

    return d["__version__"]


extensions = [
    Extension("_ctools_utils", source("ctools_utils.c"), **extra_extension_args),
    Extension("_ctools_cachemap", source("ctools_cachemap.c"), **extra_extension_args),
    Extension("_ctools_ttlcache", source("ctools_ttlcache.c"), **extra_extension_args),
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
        "Documentation": "https://github.com/ko-han/python-ctools/wiki",
        "Source Code": "https://github.com/ko-han/python-ctools",
    },
    license="Apache License 2.0",
    long_description=readme,
    long_description_content_type="text/x-rst",
    python_requires=">=3",
    include_package_data=True,
    zip_safe=False,
    ext_modules=extensions,
    packages=find_packages(include=["ctools", "ctools.*"]),
    classifiers=[
        "Programming Language :: C",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: Implementation :: CPython",
        "License :: OSI Approved :: Apache Software License",
        "Topic :: Utilities",
        "Operating System :: OS Independent",
    ],
)

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
from glob import glob
from setuptools import setup, Extension


extensions = [
    Extension("ctools", glob("src/*.c")),
]

with io.open('README.rst', 'rt', encoding='utf8') as f:
    readme = f.read()

setup(
    name="ctools",
    version="0.0.5.dev1",
    author="hanke",
    author_email="hanke0@outlook.com",
    description="A collection of useful extensions for python implement in C.",
    url="https://github.com/ko-han/python-ctools",
    license='Apache License 2.0',
    long_description=readme,
    long_description_content_type='text/x-rst',
    python_requires=">=3",
    include_package_data=True,
    zip_safe=False,
    ext_modules=extensions,
    classifiers=[
        "Programming Language :: C",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: Implementation :: CPython",
        "License :: OSI Approved :: MIT License",
        "Topic :: Utilities",
        "Operating System :: OS Independent",
    ],
)

# -*- coding: utf-8 -*-
from distutils.core import setup, Extension

extensions = [
    Extension("ctools", ["ctoolsmodule.c"]),
]

setup(
    name="ctools",
    version="0.0.1",
    author="hanke",
    author_email="hanke0@outlook.com",
    ext_modules=extensions,
    description="A toolbox for python implement in python.",
    url="https://github.com/ko-han/python-ctools",
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
)

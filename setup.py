# -*- coding: utf-8 -*-
import io
from setuptools import setup, Extension


extensions = [
    Extension("ctools", ["src/ctoolsmodule.c"]),
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
    license='MIT',
    long_description=readme,
    long_description_content_type='text/x-rst',
    python_requires=">=3",
    include_package_data=True,
    zip_safe=False,
    ext_modules=extensions,
    classifiers=[
        "Programming Language :: C",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: Implementation :: CPython"
        "License :: OSI Approved :: MIT License",
        "Topic :: Utilities",
        "Operating System :: OS Independent",
    ],
)

# -*- coding: utf-8 -*-
from distutils.core import setup, Extension

DateExt = [
    Extension("ctools", ["ctoolsmodule.c"]),
]

setup(
    name="ctools",
    version="0.0.1",
    ext_modules=DateExt
)

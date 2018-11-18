from distutils.core import setup, Extension
from Cython.Build import cythonize

DateExt = [Extension("date_ext", ["date_ext/_lib/date_ext.c"])]
CythonDateExt = cythonize("date_ext/_cython/date_ext.pyx")

setup(name="date_ext", version="1.0", ext_modules=DateExt)

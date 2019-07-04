#!/usr/bin/env bash

# Compile wheels
export CFLAGS='--std=c99'
for PYBIN in /opt/python/cp3*/bin; do
    "${PYBIN}/pip" wheel /io/ -w dist/ -v
done

# Bundle external shared libraries into the wheels
for whl in dist/*.whl; do
    auditwheel repair "$whl" --plat "$PLAT" -w /io/dist/
    rm -f "$whl"
done

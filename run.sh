#!/usr/bin/env bash

clean() {
	rm -rf dist build
	rm cmake_install.cmake CMakeCache.txt Makefile
	find . -name *.egg-info -exec rm -rf {} +
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	find . -name '*~' -exec rm -f {} +
}

compile() {
	PYTHON_HOME=$(shell which python | sed "s/\/bin\/python//")
	PYTHON_VERSION=$(shell which python | sed "s/\/bin\/python/\/include/" | xargs ls | grep python | grep python)
	gcc -DNDEBUG -O3 -Wall -Wextra -std=c99 \
		-I${PYTHON_HOME}/include \
		-I${PYTHON_HOME}/include/${PYTHON_VERSION} \
		-c ctoolsmodule.c -o build/ctools.o
}

sdist() {
	python setup.py sdist
}

bdist() {
	python setup.py bdist_wheel
	for v in '3.4' '3.5' '3.6' '3.7'; do
		python${v} setup.py bdist_wheel
	done
}

check() {
	twine check dist/*
}

test-pypi() {
	set -e
	clean
	sdist
	bdist
	check
	twine upload --skip-existing --repository-url https://test.pypi.org/legacy/ dist/*
}

install() {
	pip install .
}

tests() {
	python tests/test.py
}

benchmark() {
	python benchmarks/benchmark.py
}

case $1 in
format)
	clang-format -style=WebKit -i *.h
	clang-format -style=WebKit -i *.c
	;;
clean)
	clean
	;;
install)
	install
	;;
compile)
	compile
	;;
bdist)
	bdist
	;;
sdist)
	sdist
	;;
check)
	check
	;;
tests)
	tests
	;;
benchmark)
	benchmark
	;;
test-pypi)
	test-upload
	;;
*)
	echo >&2 Error command
	exit 1
	;;
esac

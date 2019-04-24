#!/usr/bin/env bash
set -e -x

clean() {
	rm -rf dist build
	rm -f cmake_install.cmake CMakeCache.txt Makefile
	find . -name *.egg-info -exec rm -rf {} +
	find . -name '*.pyc' -exec rm -f {} +
	find . -name '*.pyo' -exec rm -f {} +
	find . -name '*~' -exec rm -f {} +
}

compile() {
	PYTHON_HOME=$(which python | sed "s/\/bin\/python//")
	PYTHON_VERSION=$(which python | sed "s/\/bin\/python/\/include/" | xargs ls | grep python | grep python)
	gcc -DNDEBUG -O3 -Wall -std=c99 \
		-I${PYTHON_HOME}/include \
		-I${PYTHON_HOME}/include/${PYTHON_VERSION} \
		-c src/ctoolsmodule.c -o build/ctools.o
}

sdist() {
	python setup.py sdist
}

bdist() {
    export CFLAGS='--std=c99'
	for v in '3.4' '3.5' '3.6' '3.7'; do
		python${v} -m pip wheel -w dist/ .
	done
}

check() {
	twine check dist/*
}

test-pypi() {
	set -e
	check
	twine upload --repository-url https://test.pypi.org/legacy/ dist/*
}

pypi() {
	check
	twine upload dist/*
}

install() {
	pip install .
}

tests() {
	python tests/tests.py
}

benchmark() {
	python benchmarks/benchmark.py
}

wheel() {
	# Compile wheels
	export CFLAGS='--std=c99'
	for PYBIN in /opt/python/cp3*/bin; do
		"${PYBIN}/pip" wheel /io/ -w dist/
	done

	# Bundle external shared libraries into the wheels
	for whl in dist/*.whl; do
		auditwheel repair "$whl" --plat $PLAT -w /io/dist/
		rm -f $whl
	done
}

linux-bdist() {
	docker run --rm -e PLAT=manylinux1_x86_64 -v $(pwd):/io quay.io/pypa/manylinux1_x86_64 /io/run.sh wheel
	docker run --rm -e PLAT=manylinux1_i686 -v $(pwd):/io quay.io/pypa/manylinux1_x86_64 linux32 /io/run.sh wheel
	docker run --rm -e PLAT=manylinux2010_x86_64 -v $(pwd):/io quay.io/pypa/manylinux2010_x86_64 /io/run.sh wheel
}

case $1 in
format)
	clang-format -style=google -i src/*.h
	clang-format -style=google -i src/*.c
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
	test-pypi
	;;
pypi)
	pypi
	;;
wheel)
	wheel
	;;
linux-bdist)
    clean
	linux-bdist
	;;
*)
	echo >&2 Error command
	exit 1
	;;
esac

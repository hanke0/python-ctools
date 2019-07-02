#!/usr/bin/env bash

sdist() {
	python setup.py sdist
}

bdist() {
    export CFLAGS='--std=c99'
    python -m pip wheel -w dist/ .
	for v in '3.5' '3.6' '3.7'; do
		type python${v} && python${v} -m pip wheel -w dist/ .
	done
}

check() {
	twine check dist/*
}


linux-bdist() {
	sudo docker run --rm -e PLAT=manylinux1_x86_64 -v $(pwd):/io quay.io/pypa/manylinux1_x86_64 /io/tools/linux-wheel.sh
	sudo docker run --rm -e PLAT=manylinux1_i686 -v $(pwd):/io quay.io/pypa/manylinux1_i686 linux32 /io/tools/linux-wheel.sh
	sudo docker run --rm -e PLAT=manylinux2010_x86_64 -v $(pwd):/io quay.io/pypa/manylinux2010_x86_64 /io/tools/linux-wheel.sh
	sudo chown -R $USER:$USER $(pwd)
}

type docker && linux-bdist

sdist
bdist
check
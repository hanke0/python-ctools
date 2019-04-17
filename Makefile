.PHONY: format
format:  ## Inplace format all c code
	@clang-format -style=WebKit -i *.h
	@clang-format -style=WebKit -i *.c

.PHONY:
clean:  ## Delete templory and build files
	@rm -rf dist build
	@find . -name *.egg-info -exec rm -rf {} +
	@find . -name '*.pyc' -exec rm -f {} +
	@find . -name '*.pyo' -exec rm -f {} +
	@find . -name '*~' -exec rm -f {} +

.PHONY:
build: clean ## Build sdist and bdist package
	@python setup.py sdist
	@python setup.py bdist_wheel


PYTHON_HOME=$(shell which python | sed "s/\/bin\/python//")
PYTHON_VERSION=$(shell which python | sed "s/\/bin\/python/\/include/" | xargs ls | grep python | grep python)

.PHONE:
compile:
	gcc -DNDEBUG -g -fwrapv -O3 -Wall -Wextra -std=c99 -arch x86_64 \
	-I$(PYTHON_HOME)/include \
	-I$(PYTHON_HOME)/include/$(PYTHON_VERSION) \
	-c ctoolsmodule.c


.PHONY:
check:  ## Check distribution files
	@twine check dist/*

.PHONY:
test-upload:  ## Upload to test pypi
	@twine upload --skip-existing --repository-url https://test.pypi.org/legacy/ dist/*

.PHONY:
test-release: clean build check test-upload  ## Clean build and upload to test pypi

.PHONY:
upload:  ## Upload to pypi
	@twine upload --skip-existing dist/*

.PHONY:
release: clean build check upload ## Clean build and upload to pypi

.PHONY:
test:  ## Run test.py
	@python tests.py

.PHONY:
benchmark:  ## Run benchmark.py
	@python benchmark.py


.PHONY:
install:  ## Install use pip
	@pip install -v .

.PHONY:
docker-build:
	@docker build -t ctools .

.PHONE:
docker-run:
	@docker run -it --rm -v $(pwd):/tmp --network=host ctools bash


BDIST_ARGS := setup.py bdist_wheel

.PHONE:
bdist:
	@python3.4 $(BDIST_ARGS)
	@python3.5 $(BDIST_ARGS)
	@python3.6 $(BDIST_ARGS)
	@python3.7 $(BDIST_ARGS)

# Absolutely awesome: http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help
help:
	@echo Select command:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.DEFAULT_GOAL := help

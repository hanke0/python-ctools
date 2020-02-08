.DEFAULT_GOAL := build

# Absolutely awesome: http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help
help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.PHONY: dist
dist:  ## Build dists
	@$(CURDIR)/tools/clean-cache.sh
	@$(CURDIR)/tools/dist.sh
	@$(CURDIR)/tools/clean-cache.sh soft

.PHONY: clean
clean:  ## Clean cache, include dist
	find -name *.egg-info -exec rm -rf {} +
	find -name *.pyc -exec rm -rf {} +
	find -name *.pyo -exec rm -rf {} +
	find -name *.so -exec rm -rf {} +
	rm -rf dist
	rm -rf build

.PHONY: format
format:  ## Format code
	@clang-format --verbose -style=LLVM -i src/*
	@black **/*.py

.PHONY: benchmark
benchmark:  ## Run benchmark
	@$(CURDIR)/tools/runbenchmark.py benchmarks/

.PHONY: pypi-test
pypi-test:
	@$(CURDIR)/tools/upload-pypi.sh test

.PHONY: pypi
pypi:
	@$(CURDIR)/tools/upload-pypi.sh

.PHONY: build
build: clean  ## build package
	@python setup.py build_ext --inplace

.PHONY: test
test:  ## running test
	@$(CURDIR)/tools/runtest.py -s ./tests -p .

.PHONY: doc
doc: build ## genetate api doc
	cd docs && make clean && make html

.PHONY: check
check:
	clang-tidy src/* -- -I/usr/include/python3.7m/

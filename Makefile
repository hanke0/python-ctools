PYTHON ?= python

.DEFAULT_GOAL := build

HELP_LINE="  \033[36m%-30s\033[0m %s\n"
# Absolutely awesome: http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help
help:
	@echo "CTools Makefile"
	@echo
	@echo "Variables:"
	@printf $(HELP_LINE) PYTHON "python executable path"
	@echo
	@echo "Commands:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf $(HELP_LINE), $$1, $$2}'

.PHONY: dist
dist: clean  ## Build dists
	@$(CURDIR)/tools/dist.sh
	@$(MAKE) clean-cache

.PHONY: clean-cache
clean-cache:
	@find -name *.egg-info -exec rm -rf {} +
	@find -name *.pyc -exec rm -rf {} +
	@find -name *.pyo -exec rm -rf {} +
	@find -name *.so -exec rm -rf {} +
	@find -name *.pyd -exec rm -rf {} +

.PHONY: clean
clean: clean-cache ## Clean cache, include dist
	@rm -rf dist
	@rm -rf build

.PHONY: format fmt
format fmt:  ## Format code
	@clang-format --verbose -i src/*
	@black **/*.py
	@black *.py

.PHONY: benchmark
benchmark:  ## Run benchmark
	@$(PYTHON) $(CURDIR)/tools/runbenchmark.py benchmarks/

.PHONY: pypi-test
pypi-test:
	@$(CURDIR)/tools/upload-pypi.sh test

.PHONY: pypi
pypi:
	@$(CURDIR)/tools/upload-pypi.sh

.PHONY: build
build: clean  ## Build package
	@$(PYTHON) setup.py build_ext --inplace

.PHONY: test
test:  ## Run unit tests
	@$(PYTHON) $(CURDIR)/tools/runtest.py -s ./tests -p .

.PHONY: check-doc
check-doc:  ## Check documentation
	$(PYTHON) $(CURDIR)/tools/checkdoc.py 'ctools.**'

.PHONY: doc
doc: ## Build documentation
	cd $(CURDIR)/docs && make clean && make html

PY_INCLUDE=$(shell $(PYTHON) -c "from sysconfig import get_paths as gp; print(gp()['include'])")
.PHONY: check
check:  ## Clang-tidy codes
	clang-tidy src/* -- -I "$(PY_INCLUDE)"

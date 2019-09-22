.DEFAULT_GOAL := help

# Absolutely awesome: http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

dist:  ## Build dists
	@$(CURDIR)/tools/clean-cache.sh
	@$(CURDIR)/tools/dist.sh
	@$(CURDIR)/tools/clean-cache.sh soft

clean:  ## Clean cache, include dist
	@$(CURDIR)/tools/clean-cache.sh

format:  ## Format code
	@clang-format -style=Mozilla -i src/*.h
	@clang-format -style=Mozilla -i src/*.c
	@black **/*.py

benchmark:  ## Run benchmark
	@$(CURDIR)/tools/runbenchmark.py benchmarks/

pypi-test:
	@$(CURDIR)/tools/upload-pypi.sh test

pypi:
	@$(CURDIR)/tools/upload-pypi.sh

build:  ## build package
	@python setup.py build_ext --inplace

test: build  ## Install package and Test
	@$(CURDIR)/tools/runtest.py

doc:  ## genetate api doc
	@$(CURDIR)/tools/genapidoc.py

.PHONY: help dist clean format benchmark pypi-test pypi build test doc

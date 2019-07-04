.DEFAULT_GOAL := help

# Absolutely awesome: http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

dist:  ## Build dists
	@tools/clean-cache.sh
	@tools/dist.sh
	@tools/clean-cache.sh soft

clean:  ## Clean cache, include dist
	@tools/clean-cache.sh

format:  ## Format code
	@clang-format -style=Mozilla -i src/*.h
	@clang-format -style=Mozilla -i src/*.c

benchmark:  ## Run benchmark
	@python tools/runbenchmark.py benchmarks/

pypi-test:
	@tools/upload-pypi.sh test

pypi:
	@tools/upload-pypi.sh

install:  ## Install package
	@pip install . -vvv

test:  ## Install package and Test
	@python tools/runtest.py

doc:  ## genetate api doc
	@python tools/genapidoc.py

.PHONY: help dist clean format benchmark pypi-test pypi install test doc

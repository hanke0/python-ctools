.PHONY: format
format:  ## inplace format all c code
	@clang-format -style=WebKit -i *.h
	@clang-format -style=WebKit -i *.c

.PHONY:
clean:  ## delete templory and build files
	@rm -rf dist build
	@find . -name *.egg-info -exec rm -rf {} +
	@find . -name '*.pyc' -exec rm -f {} +
	@find . -name '*.pyo' -exec rm -f {} +
	@find . -name '*~' -exec rm -f {} +

.PHONY:
build:  ## build sdist and bdist package
	@python setup.py sdist
	@python setup.py bdist_wheel

.PHONY:
release-test: clean build test-upload  ## clean build and upload to test pypi

.PHONY:
test-upload:  ## upload to test pypi
	@twine upload --skip-existing --repository-url https://test.pypi.org/legacy/ dist/*

.PHONY:
upload:  ## upload to pypi
	@twine upload --skip-existing dist/*

.PHONY:
release: clean build upload ## clean build and upload to pypi


# Absolutely awesome: http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help
help:
	@echo Select command:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.DEFAULT_GOAL := help

#!/usr/bin/env bash

set -e
twine check dist/*
case $1 in
    test)
        twine upload --repository-url https://test.pypi.org/legacy/ dist/*
        ;;
    *)
        twine upload dist/*
        ;;
esac

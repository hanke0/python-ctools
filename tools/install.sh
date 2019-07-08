#!/usr/bin/env bash

HERE=$(dirname $0)
PROJECT_ROOT=$(dirname ${HERE})

pip -V
echo pip install ${PROJECT_ROOT} -v
pip install ${PROJECT_ROOT} -v


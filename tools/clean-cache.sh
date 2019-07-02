#!/usr/bin/env bash
set -e

HERE=$(dirname $0)
PROJECT_ROOT=$(dirname ${HERE})

FILES="$(find ${PROJECT_ROOT} -name '*.egg-info') $(find ${PROJECT_ROOT} -name '*.pyc') $(find ${PROJECT_ROOT} -name '*.pyo') $(find ${PROJECT_ROOT} -name '*~')"

_read_yes() {
    echo -n "$1[Yes/no] "
    read input
    case ${input} in
    [yY][eE][sS] | [yY])
        return 0
        ;;
    *)
        return 1
        ;;
    esac
}


clean() {
    echo "While delete files:"
    echo ${FILES} |  tr '[:space:]' '\n'
    echo -n "Continue? [Y/N]: "
    answer=n
    read answer
    if [[ "$answer" != "${answer#[Yy]}" ]]; then
        echo delete
        rm -rf ${FILES}
    fi
}

case $1 in
    soft)
        clean
        ;;
    *)
        FILES="$FILES ${PROJECT_ROOT}/build ${PROJECT_ROOT}/dist"
        clean
        ;;
esac

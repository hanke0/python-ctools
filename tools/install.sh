#!/usr/bin/env bash

HERE=$(dirname $0)
PROJECT_ROOT=$(dirname ${HERE})
VERSION_FILE=${PROJECT_ROOT}/ctools/version.py

old_version=$(cat ${VERSION_FILE})

if [[ ${CTOOLS_DEBUG} == "ON" ]]; then
    echo "write git commit version info to " ${VERSION_FILE}
    python ${HERE}/githeadversion.py
fi
pip -V
pip install . -v

echo "Restore version file" ${VERSION_FILE}
cat > ${VERSION_FILE} <<EOF
${old_version}
EOF

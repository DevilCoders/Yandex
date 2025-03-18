#!/bin/bash

set -e

project_path="`cd $(dirname $0)/.. && pwd`"

resources_path="//home/mstand/resources"
python_minor_ver=$(python3 -c"import sys; print(sys.version_info.minor)")

echo "build bindings.so"
cd "${project_path}/bin/turbo_urls"

echo "for python 3.6"
ya make -j4 -r --checkout -DUSE_SYSTEM_PYTHON=3.6
strip bindings.so
if python3.6 -V &> /dev/null; then
    python3.6 -c "import bindings"
elif [ ${python_minor_ver} = 6 ]; then # if there is an activated virtual environment with python 3.6
    python3 -c "import bindings"
fi
echo "upload bindings.so to $resources_path/bindings.so..."
cat bindings.so | yt upload "$resources_path/bindings.so"

echo "for python 3.8"
ya make -j4 -r --checkout -DUSE_SYSTEM_PYTHON=3.8 -DCFLAGS="-Wno-missing-field-initializers -Wno-deprecated-declarations"
strip bindings.so
if python3.8 -V &> /dev/null; then
    python3.8 -c "import bindings"
elif [ ${python_minor_ver} = 8 ]; then # if there is an activated virtual environment with python 3.8
    python3 -c "import bindings"
fi
echo "upload bindings.so to $resources_path/py3.8/bindings.so..."
cat bindings.so | yt upload "$resources_path/py3.8/bindings.so"

#!/bin/bash

QUIET_MODE="--quiet --quiet"
apt-get ${QUIET_MODE} install --no-install-recommends -y nodejs

JUPYTER_CONFIG_DIR="/etc/jupyter"
JUPYTER_CONFIG=${JUPYTER_CONFIG_DIR}"/jupyter_notebook_config.py"

[ -d ${JUPYTER_CONFIG_DIR} ] || sudo mkdir -p ${JUPYTER_CONFIG_DIR}

if [ ! -f ${JUPYTER_CONFIG} ]; then
    sudo sh -c "echo \"c.NotebookApp.ip = '0.0.0.0'\" > ${JUPYTER_CONFIG}"
fi

# Error
# https://github.com/Anaconda-Platform/nb_conda/issues/66

# Basic kernels
## Bash kernel
# https://github.com/takluyver/bash_kernel
pip install bash_kernel
python -m bash_kernel.install

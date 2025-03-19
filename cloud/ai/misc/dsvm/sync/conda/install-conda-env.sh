#!/bin/bash
set -ev

CONDA_PACKAGES="ipykernel tensorflow conda-forge::catboost conda-forge::lightgbm  conda-forge::xgboost pandas ipython matplotlib pytorch::pytorch pytorch::torchvision intel::scikit-learn"

conda update conda --quiet --yes
conda update --all --quiet --yes

##########################################################################################

# python 3.6
conda create --prefix /opt/conda/envs/py36 -c intel intelpython3_core python=3.6

# from intel distribution
# see https://blog.kovalevskyi.com/deeplearning-images-revision-m9-intel-optimized-images-273164612e93

conda install -n py36 ${CONDA_PACKAGES}

# optimized jpeg processing
conda uninstall -n py36 --force jpeg libtiff pillow -y
conda install -n py36 conda-forge::libjpeg-turbo

source activate py36
pip install --ignore-installed --upgrade pip
CC="cc -mavx2" pip install --no-cache-dir -U --force-reinstall pillow-simd
# pip install --upgrade-strategy only-if-needed --no-cache-dir ${PIP_PACKAGES}
source deactivate

##########################################################################################

# python 2.7
conda create --prefix /opt/conda/envs/py27 -c intel intelpython2_core python=2.7
conda install -n py27 ${CONDA_PACKAGES}

# optimized jpeg processing
conda uninstall -n py27 --force jpeg libtiff pillow -y
conda install -n py27 conda-forge::libjpeg-turbo

source activate py27
pip install --ignore-installed --upgrade pip
CC="cc -mavx2" pip install --no-cache-dir -U --force-reinstall pillow-simd
# pip install --upgrade-strategy only-if-needed --no-cache-dir ${PIP_PACKAGES} tensorflow-serving-api
source deactivate


##########################################################################################

conda install nb_conda conda-forge::jupyter_contrib_nbextensions conda-forge::jupyter_nbextensions_configurator

# install jupyterlab
conda install jupyter jupyterlab

# Remove unused packages and caches
conda clean --all --yes


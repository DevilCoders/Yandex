#!/bin/bash
set -ev

git clone https://github.com/tensorflow/tensorflow.git


source activate py27
python tensorflow/tensorflow/examples/tutorials/mnist/mnist.py || exit 1
source deactivate

source activate py36
python tensorflow/tensorflow/examples/tutorials/mnist/mnist.py || exit 1
source deactivate

rm -rf tensorflow

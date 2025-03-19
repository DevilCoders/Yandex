#!/bin/bash

cd /tmp
git clone https://github.com/pytorch/examples.git pytorch_examples
cd pytorch_examples

source activate py27
python mnist/main.py --epochs 2 || exit 1
source deactivate

source activate py36
python mnist/main.py --epochs 2 || exit 1
source deactivate

cd ..
rm -rf pytorch_examples

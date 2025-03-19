#!/bin/bash
set -e

# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/tutorials/word2vec/word2vec_basic.py
bash -c "source activate py27; MPLBACKEND="agg" python ./word2vec_basic.py"
bash -c "source activate py36; MPLBACKEND="agg" python ./word2vec_basic.py"


#!/bin/bash

echo "$1"
python ml_churn_prediction.py "$1"
echo results_done
source /home/lunin-dv/anaconda3/bin/activate python2
#python2 --version
python2 results_savior.py "$1"

#!/bin/sh

mkdir xgboost-src
cd xgboost-src

git clone --recursive https://github.com/dmlc/xgboost.git
tar -cvzf xgboost.tgz xgboost

ya upload --ttl 730 xgboost.tgz

#!/usr/bin/env bash

argc=$#

if [ $argc -ne 3 ];
then
    echo "Usage: $0 learn_features_markup.json test_features_markup.json stuff_directory"
    echo "  learn_eatures_markup.json & test_features_markup.json - http://wiki.yandex-team.ru/ynews/doc/tech/pr_meter#formatyrazmetok"
    echo "  stuff_directory - directory with files: 2ld.list dict.dict htparser.ini" #todo: write where they can be found
    exit 42
fi

stuff_dir=$3
learn_markup=$1
test_markup=$2
learn_features="$learn_markup.features"
test_features="$test_markup.features"
learn_prediction="$learn_markup.prediction"
test_prediction="$test_markup.prediction"

EXIT_STATUS=0

echo "generating test features..." || EXIT_STATUS=$?
./mains_features -r json -a $test_markup -c $stuff_dir/ > $test_features || EXIT_STATUS=$?

echo "generating learn features..." || EXIT_STATUS=$?
./mains_features -r json -a $learn_markup -c $stuff_dir/ > $learn_features || EXIT_STATUS=$?

echo "running machine learning..." || EXIT_STATUS=$?
./reg_tree2 -b 40 -s 10000 -T 32 -f $learn_features -t $test_features > learning.log || EXIT_STATUS=$?

echo "applying..." || EXIT_STATUS=$?
./reg_tree2 --apply -f $test_features > $test_prediction || EXIT_STATUS=$?
./reg_tree2 --apply -f $learn_features > $learn_prediction || EXIT_STATUS=$?

echo "detecting thresholds..." || EXIT_STATUS=$?

./tune_threshold --learn $learn_prediction --test $test_prediction

exit $EXIT_STATUS


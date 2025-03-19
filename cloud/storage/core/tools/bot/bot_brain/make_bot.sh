#!/usr/bin/env bash
./build_training_set.py -i chat_history.json -l learn.txt -t test.txt &&
~/arc/nbs/arcadia/ml/neocortex/neocortex/neocortex "Mode=TextToText" "Learn=learn.txt" "Test=test.txt" "Type=words(Size=304)+firstngram(Size=80)+contrastwords" "Type2=words(Size=304)+firstngram(Size=80)+contrastwords" "Iterations=20" "L2RegCoeff=2e-4" "LearningRate=0.04" "NumLearnNegatives=8" &&
cat learn.txt test.txt | awk -F '\t' '{print $1; print $2}' | sort | uniq > answers.txt

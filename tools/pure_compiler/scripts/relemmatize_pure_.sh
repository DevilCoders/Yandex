#!/usr/bin/env sh

./lemmer-test --fingerprints > $3_fingerprints.txt
cat $1 | python lemmatize_pure.py | LC_ALL=C sort | python glue_pure_lemmatized.py | LC_ALL=C sort | python collapse_pure.py | LC_ALL=C sort > $2
./pure_compiler -blc -f $3_fingerprints.txt -i $2 $3

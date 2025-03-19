word dists
=============

DATE=20190107

python ./extract_queries.py --date $DATE --output //home/search-functionality/antonio/tmp/extract_queries_output --normalizer-path ~/normalizelib.so --gzt-path ~/special_words.gzt.bin --patterns-path ~/all_patterns.txt

split_by_words.py

build_weights.py

filter_result.py

./filter_by_freq


bigram dists
==============
DATE=20190107

python ./extract_queries.py --date $DATE --output //home/search-functionality/antonio/tmp/extract_queries_output --normalizer-path ~/normalizelib.so --gzt-path ~/special_words.gzt.bin --patterns-path ~/all_patterns.txt

./prepare_words_and_bigrams --pure-file ./pure.lg.groupedtrie.rus -i //home/search-functionality/antonio/tmp/extract_queries_output -o //home/search-functionality/antonio/tmp/prepare_words_and_bigrams_output --threshold 1000

python ./build_paris_with_factors.py -i //home/search-functionality/antonio/tmp/prepare_words_and_bigrams_output --output //home/search-functionality/antonio/tmp/build_paris_with_factors_output
(https://nirvana.yandex-team.ru/flow/08ac29af-9570-4566-ba1f-2dbd8f4f713e)

python ./build_weights.py -i //home/search-functionality/antonio/tmp/build_paris_with_factors_output --output //home/search-functionality/antonio/tmp/build_weights_output
(https://nirvana.yandex-team.ru/flow/b54b7142-7cf2-4010-a0dd-4aadb16645e0)

./build_trie -i //home/search-functionality/antonio/tmp/build_weights_output -o ~/trie.bin

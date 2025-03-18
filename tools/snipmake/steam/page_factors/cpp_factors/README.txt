Scripts for learning and quality checking of segmentator.

INFO:
qualifier_data: https://arc.yandex-team.ru/wsvn/arc/trunk/arcadia_tests_data/segmentator_tests_data/segmentator2015/qualifier_data/

1. Fetching docs with guids from STEAM.

python ./get_guided_docs.py --help
optional arguments:
  -h, --help         show this help message and exit
  -l DOCS_LIST_FILE  - (default: docs_list.txt)
  -d DOCS_DIR        - (default: guided_docs)
  -s SERVER          - (default: https://mcquack.search.yandex.net:8043)

DOCS_LIST_FILE - each line in the format: "%d+.txt\t%s" % (doc_id, url)
    doc_id - incremental integer (start id is 0)
DOCS_DIR - dir to store guided docs
SERVER - STEAM host for fetching

Example:
python ./get_guided_docs.py


2. Learning 3 new formulas (replace old!) for segmentator with STEAM estimations,
    recompile segmentator_tool with new formulas.

python ./learner.py --help
usage: learner.py [-h] [-m MXNET_FORMULAS_DIR] [-t TMP_FACTORS_DIR]
                  [-d DOCS_DIR] [-l DOCS_LIST_FILE] [-a DOCS_ANS_FILE] --dict
                  REC_DICT_FILE
optional arguments:
  -h, --help            show this help message and exit
  -m MXNET_FORMULAS_DIR
                        - (default: segmentator_tool/mxnet_top100)
  -t TMP_FACTORS_DIR    - (default: tmp_learner_factors)
  -d DOCS_DIR           - (default: guided_docs)
  -l DOCS_LIST_FILE     info: must contain docs in the corresponding order for
                        DOCS_ANS_FILE (default: docs_list.txt)
  -a DOCS_ANS_FILE      Downloaded from STEAM file with estimations (default:
                        qualifier_data/human_ests.tsv)
  --dict REC_DICT_FILE  recognizer dict file (default: None)

MXNET_FORMULAS_DIR - dir with 3 formulas (see
    tools/snipmake/steam/page_factors/cpp_factors/segmentator_tool/mxnet_top100)
TMP_FACTORS_DIR - dir for temporary files
DOCS_DIR - dir with guided docs (see p.1)
DOCS_LIST_FILE - file with ids and urls of docs (see p.1)
DOCS_ANS_FILE - file with answers from STEAM (use
    tools/snipmake/steam/page_factors/ests_to_steam.py to get it)
REC_DICT_FILE - file arcadia_tests_data/recognize/dict.dict

NOTE: DOCS_LIST_FILE must contain the same order of docs as DOCS_ANS_FILE

Example:
python learner.py --dict ~a-bocharov/arcadia_tests_data/recognize/dict.dict > out.txt 2> err.txt


3. Aggregate error-file statistics from learner.py (merge stage).

python ./learner_merge_err_stats.py --help
usage: learner_merge_err_stats.py [-h] [-f ERR_FILE]
optional arguments:
  -h, --help   show this help message and exit
  -f ERR_FILE  - (default: err.txt)

ERR_FILE - file with stderr-logs of learner.py (see p.2)

Example:
python ./learner_merge_err_stats.py -f err.txt
docs_count: 229.000000
abs_try_parent: 59.441048
abs_guids: 323.646288
abs_ok: 35.899563
abs_wrong_nodes: 12.253275
abs_not_found: 275.493450
rel_try_parent: 0.183015
rel_guids: 1.000000
rel_ok: 0.115102
rel_wrong_nodes: 0.039206
rel_not_found: 0.845693

abs - absolute value; rel - relative value
try_parent - can't find node from arcadia-tree in STEAM-tree,
    try to find node's parent
guids - amount of nodes for merge
ok - merge id of node is correct (equals to prev or prev + 1)
wrong_nodes - merge id of node is incorrect
not_found - can't find node and node's parent
NOTE: each value is normalized (divided by docs_count)


4. Calculate quality of test and production segmentators versions with
    STEAM estimations.

python ./qualifier.py --help
usage: qualifier.py [-h] [-c HUMAN_SENTS_FILE] [-p PROD_SENTS_FILE]
                    [-e HUMAN_ESTS_FILE] [-t TMP_SEGM_DIR] [-d DOCS_DIR]
                    --dict REC_DICT_FILE
optional arguments:
  -h, --help            show this help message and exit
  -c HUMAN_SENTS_FILE   - (default: qualifier_data/human_sents.txt)
  -p PROD_SENTS_FILE    - (default: qualifier_data/prod_sents.txt)
  -e HUMAN_ESTS_FILE    - (default: qualifier_data/human_ests.tsv)
  -t TMP_SEGM_DIR       - (default: tmp_qualifier_segm)
  -d DOCS_DIR           - (default: qualifier_data/docs)
  --dict REC_DICT_FILE  recognizer dict file (default: None)

HUMAN_SENTS_FILE - file with correct sentences
PROD_SENTS_FILE - file with sentences from production segmentator (use
    tools/segutils/tests/segmentator_test/segmentator_test to get it)
HUMAN_ESTS_FILE - file with answers from STEAM (use
    tools/snipmake/steam/page_factors/ests_to_steam.py to get it),
    this script uses only ids from the file
TMP_SEGM_DIR - dir for temporary files
DOCS_DIR - dir with docs for checking quality
REC_DICT_FILE - file arcadia_tests_data/recognize/dict.dict

Example:
python qualifier.py --dict ~a-bocharov/arcadia_tests_data/recognize/dict.dict > tmp_qualifier_segm/run.log


#!/usr/bin/env bash

if [[ $1 == -h || $1 == --help || $1 == --usage ]]; then
    echo "Usage: $0 [out_file]"
    echo "export DICT_ROOT=... to use pre-checked-out svn+ssh://svn.yandex-team.ru/dictionaries/trunk/morph"
    exit -1
fi

set -e
set -o pipefail

OUT="${1-../rus_fio/rus.dict.bin.famn.m_f.gz}"
CUR_DIR=$(readlink -f $(dirname $0))
cd "$CUR_DIR"
ARCADIA_ROOT=$(readlink -f $CUR_DIR/../../../..)
YA=$ARCADIA_ROOT/ya
SVN="$YA tool svn"
DICT_REPOSITORY="svn+ssh://arcadia.yandex.ru/robots/trunk/dictionaries/morph/rus"
: ${DICT_ROOT:=$CUR_DIR/rus}

_status="\033[31;1mFailed to generate the dictionary\033[0m\n
Make sure morph/rus is your DICT_ROOT
DICT_ROOT: $DICT_ROOT
SVN URL: $DICT_REPOSITORY\n
Leave DICT_ROOT empty to check it out to '.' automatically."

bye() { echo -e "$_status"; [[ $_error ]] && echo -e "\033[37;40m$_error\033[0m"; }
trap bye EXIT

PREPRUSDICT=$ARCADIA_ROOT/dict/morphdict/rusbyk/preprusdict.py
MAKE_MORPHDICT=$ARCADIA_ROOT/dict/tools/make_morphdict/main/make_morphdict

if [ ! -d $DICT_ROOT ]; then
    _error="svn co failed. See https://st.yandex-team.ru/DEVTOOLSSUPPORT-18275"
    $SVN checkout $DICT_REPOSITORY $DICT_ROOT
    _error=
else
    echo "Using dictionaries from $DICT_ROOT"
fi

[[ -f $PREPRUSDICT ]] || $SVN update --parents $PREPRUSDICT
[[ -x $MAKE_MORPHDICT ]] || $YA make -r --checkout $(dirname $MAKE_MORPHDICT)

TMP_RAW_LISTER=$(mktemp)
TMP_RAW_LISTER_1=$(mktemp)
TMP_PREP_LISTER=$(mktemp)

trap "bye; rm -f $TMP_RAW_LISTER $TMP_RAW_LISTER_1 $TMP_PREP_LISTER" EXIT

echo "[1 / 4] famn.sh..."
bash famn.sh ${DICT_ROOT}/main/rus-lister.txt ${TMP_RAW_LISTER}

echo "[2 / 4] $PREPRUSDICT..."
cat ${TMP_RAW_LISTER} ${DICT_ROOT}/famn/names-lt-m.txt ${DICT_ROOT}/famn/names-lt-f.txt | \
    python ${PREPRUSDICT} -d \
    > ${TMP_RAW_LISTER_1}

echo "[3 / 4] split_iev.py..."
cat ${TMP_RAW_LISTER_1} | python split_iev.py > ${TMP_PREP_LISTER}

echo "[4 / 4] $MAKE_MORPHDICT..."
$MAKE_MORPHDICT make-all --language rus --bastard-freq 0 --fio-mode <$TMP_PREP_LISTER | gzip -f --fast >$OUT
_status="\033[32;1mAll operations succeeded, the output is $OUT\033[0m"

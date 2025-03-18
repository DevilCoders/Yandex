#!/bin/sh
alias warn="echo 1>&2" 

RunOrKill() {
    local RESULT
    "$@"
    RESULT=$?
    if [ $RESULT -ne 0 ]
    then
        warn "Error: $@ returned $RESULT."
        kill $$
        exit 1
    fi
}

if [ $# -ne 2 ]
then
    warn "Usage: $0 INPUT OUTPUT"
    exit 1
fi
INPUT=$1
OUTPUT=$2
SORTED_FILE=$OUTPUT.bigrams.sorted
export LC_ALL=C
cat $INPUT | tr "_" "\t" | RunOrKill sort > $SORTED_FILE &
WORDS_TRIE=$OUTPUT.words.trie
RunOrKill awk -f print_words.awk $INPUT | RunOrKill sort -u | RunOrKill awk -f number_words.awk | RunOrKill ../triecompiler/triecompiler -wsfqt ui32 $WORDS_TRIE
wait
RunOrKill ./bigram_compiler -w $WORDS_TRIE -o $OUTPUT -i $SORTED_FILE 
rm $SORTED_FILE $WORDS_TRIE

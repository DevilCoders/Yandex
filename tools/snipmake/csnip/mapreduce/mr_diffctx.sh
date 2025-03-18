#!/bin/bash
EXP=`cat exp.txt`
./csnip -j 1 $EXP -r mr_diffctx
#./csnip -j 1 $EXP -r mr_diffstat
if [ $? -ne 0 ]; then
    echo "Error"
    # read stdin
    cat > /dev/null
fi



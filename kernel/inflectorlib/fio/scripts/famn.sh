cat $1 | grep -v "@" | grep -B2 -E "famn|persn|patrn" | grep -v "\-\-" | python split_mf.py > $2

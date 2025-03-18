join -1 1 -2 1 matrixnet.fstr factor_names.txt | cut -f 2,5,6 -d " " | grep -v "0 " | grep -v "e-" | sort -rn

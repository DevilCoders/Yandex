#!/bin/bash
DATA_PATH='/var/lib/mongo-backup/'
END_DATE=0
CODE=-1
if [[ -z $1 ]]; then
    echo "1; Backup name and MAX_BKP_AGE not given."
    exit 1
fi

if [[ ! -f "$DATA_PATH/$1.rslt" ]]; then
    echo "1; No .rslt file. Was any backups done?"
    exit 1
fi

if [[ -z $2 ]]; then
    echo "1; MAX_BKP_AGE not given."
    exit 1
fi

case ${2: -1:1} in 
    [hH]) MAX_BKP_AGE=$((${2%?} * 3600))
          ;;
    [mM]) MAX_BKP_AGE=$((${2%?} * 60))
          ;; 
    [0-9]) MAX_BKP_AGE=$2
          ;;
     *) echo "1; Invalid MAX_BKP_AGE"
          exit 1
          ;; 
esac

read -d '\n' -a KNOWN_CODES < "$DATA_PATH/known-codes"
KNOWN_CODES+=(255)
KNOWN_CODES+=(-1)
[[ -f "$DATA_PATH/$1.exit" ]] && read CODE END_DATE < "$DATA_PATH/$1.exit"

read START_DATE START_MSG < <(head -1 "$DATA_PATH/$1.rslt")
read LAST_DATE LAST_STATUS LAST_MSG < <(sort -k 2,2 -s "$DATA_PATH/$1.rslt" | tail -1)
if [[ $START_DATE -gt $END_DATE ]]; then
    echo "$LAST_STATUS $LAST_MSG"
    exit 0
fi

UNKNOWN=1
for i in ${KNOWN_CODES[@]}; do
    if [[ $i -eq $CODE ]]; then
        UNKNOWN=0
        break
    fi
done

if [[ $UNKNOWN -eq 1 ]]; then
    echo "2; Unknown exit code"
    exit 1
fi 

BKP_AGE=$(($(date +%s) - $END_DATE))

if [[ $BKP_AGE -gt $MAX_BKP_AGE ]]; then
    echo "2; Too old."
    exit 1
fi

echo "$LAST_STATUS $LAST_MSG"
[[ $LAST_STATUS == "0;" ]] && exit 0
exit 1

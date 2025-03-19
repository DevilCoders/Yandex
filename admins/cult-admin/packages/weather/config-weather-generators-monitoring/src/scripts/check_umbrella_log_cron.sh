LOG="/var/log/yandex/umbrella/umbrella.log"
LOG_COUNTER="/tmp/umbrella-errcont.tmp"
TIME="3600"
# Renew tmp log file
rm $LOG_COUNTER

if [[ -f "$LOG" ]]; then
        timetail -n $TIME -t java $LOG | grep -e "ERROR" | wc -l > $LOG_COUNTER
else
        exit 1
fi

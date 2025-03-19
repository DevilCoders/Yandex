{% from "components/greenplum/map.jinja" import gpdbvars with context %}
#!/bin/bash

################################################################################################
# Objective  : To identify a  query for a specific interval
# Summary    : This identifies the pid and kills him
# Logging    : All logs will be stored in ${HOME}/arenadata_configs/long_query_time_kill.log
# Consequences  :
#            Connections killed will get the below error messages and they would need to reconnect
#            FATAL:  terminating connection due to administrator command
#            server closed the connection unexpectedly
#                This probably means the server terminated abnormally
#                before or while processing the request.
#            The connection to the server was lost. Attempting reset: Succeeded.
################################################################################################

source ${HOME}/.bashrc

ACTION=1
INTERVAL_VALUE=24
INTERVAL_TYPE="hours"
USER=""
INCLUDE=0

printHelp() {
    echo "
    Usage:
    kill_idle_in_transaction.sh [OPTION]
    -h    : Display help
    -l    : Only logging query in $LOGFILE
    -k    : Kill querys
    -v    : Interval value
    -i    : Interval type
    -u    : User filtering for not kill option only
    -o    : Kill querys include users

    The default action is kill (-k).
    By default, the values -v option is 24 and type for -i hours.

    Allowed value for option -i: years, months, weeks, days, hours, minutes and secs. (All PostgreSQL interval\`s).

    Examples:
    long_query_time_kill.sh [ kills querys that runs more than 24 hours ]
    long_query_time_kill.sh -k -i hours -v 10 [ kills querys that runs more than 10 hours ]
    long_query_time_kill.sh -k -u \"'bob','tom'\" -o [ kills a bob and tom querys that runs more than 24 hours]
    long_query_time_kill.sh -k -u \"'bob','tom'\" -i hours -v 10 [ kills querys that runs more than 10 hours except for users bob and tom]"

}

LOGFILE={{ gpdbvars.gplog }}/mvideo_long_query_time_kill.log


logging (){
    # Logging all query over a set interval
    psql -t -c "select pid, sess_id, usename, client_addr, query_start, (now()::timestamp - query_start::timestamp) as duration, query from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and waiting is false" template1 >> $LOGFILE
}


killQuerys () {
    # Pid generation for querys that exceed a specified interval:
    psql -A -t -c "select pg_terminate_backend(pid) from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and waiting is false" template1 | echo "$(wc -l) query killed" >> $LOGFILE
}

killQuerysExcludeUser () {
    # Pid generation for user querys that exceed a specified interval:
    # This function filtering query from the user specified in the -u option
    psql -A -t -c "select pg_terminate_backend(pid) from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and waiting is false and usename NOT IN ($USER)" template1 | echo "$(wc -l) query killed" >> $LOGFILE
}

killQuerysIncludeUser () {
    # Pid generation for user querys that exceed a specified interval:
    # This function filtering query from the user specified in the -u option
    psql -A -t -c "select pg_terminate_backend(pid) from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and waiting is false and usename IN ($USER)" template1 | echo "$(wc -l) query killed" >> $LOGFILE
}

#
# Main program
#

while getopts "v:i:u:hlko" opt ; do
    case $opt in
        k ) ACTION=1;                  ;;
        l ) ACTION=0;                  ;;
        v ) INTERVAL_VALUE=$OPTARG;    ;;
        i ) INTERVAL_TYPE="$OPTARG";   ;;
        u ) USER="$OPTARG";            ;;
        o ) INCLUDE=1;                 ;;
        h ) printHelp
            exit 1
    esac
done
shift $(($OPTIND - 1))

echo $(date)" -- Start" >> $LOGFILE

if [ "$ACTION" -eq 0 ]; then
    logging
    echo $(date)" -- End" >> $LOGFILE
    exit 0
fi

if [ "$ACTION" -eq 1 ];then
    if [ -n "$USER" ];then
        logging
        if [ "$INCLUDE" -eq 1 ];then
           killQuerysIncludeUser
        else
           killQuerysExcludeUser
        fi
    else
        logging
        killQuerys
    fi
fi

echo $(date)" -- End" >> $LOGFILE


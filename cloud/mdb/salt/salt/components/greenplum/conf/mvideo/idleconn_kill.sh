{% from "components/greenplum/map.jinja" import gpdbvars with context %}
#!/bin/bash

################################################################################################
# Objective  : To identify a <IDLE> query for a specific interval
# Summary    : This identifies the pid and kills him
# Logging    : All logs will be stored in ${HOME}/arenadata_configs/idleconn_kill.log
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
INTERVAL_VALUE=8
INTERVAL_TYPE="hours"
USER=""

printHelp() {
    echo "
    Usage:
    idleconn_kill.sh [OPTION]
    -h    : Display help
    -l    : Only logging query in $LOGFILE
    -k    : Kill querys
    -v    : Interval value
    -i    : Interval type
    -u    : User filtering for not kill option only

    The default action is kill (-k).
    By default, the values -v option is 8 and type for -i hours.

    Allowed value for option -i: years, months, weeks, days, hours, minutes and secs. (All PostgreSQL interval\`s).

    Examples:
    idleconn_kill.sh [ kills <IDLE> querys that runs more than 8 hours ]
    idleconn_kill.sh -k -i hours -v 10 [ kills <IDLE> querys that runs more than 10 hours ]
    idleconn_kill.sh -k -i hours -v 10 -u \"'bob','tom'\" [ kills <IDLE> querys that runs more than 10 hours except for users bob and tom]"
}

LOGFILE={{ gpdbvars.gplog }}/mvideo_idleconn_kill.log


logging (){
    # Logging all query over a set interval
    psql -t -c "select pid, sess_id, usename, client_addr, query_start, (now()::timestamp - query_start::timestamp) as duration, query from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and state='idle'" template1 >> $LOGFILE
}


killQuerys (){
    # Pid generation for querys that exceed a specified interval:
    psql -A -t -c "select pg_terminate_backend(pid) from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and state='idle'" template1 | echo "$(wc -l) query killed" >> $LOGFILE
}


killQuerysUser () {
    # Pid generation for user querys that exceed a specified interval:
    # This function filtering query from the user specified in the -u option
    psql -A -t -c "select pg_terminate_backend(pid) from pg_stat_activity where (now()::timestamp - query_start::timestamp) > interval '$INTERVAL_VALUE $INTERVAL_TYPE' and state='idle' and usename NOT IN ($USER)" template1 | echo "$(wc -l) query killed" >> $LOGFILE
}

#
# Main program
#

while getopts "v:i:u:hlk" opt ; do
    case $opt in
        k ) ACTION=1;                  ;;
        l ) ACTION=0;                  ;;
        v ) INTERVAL_VALUE=$OPTARG;    ;;
        i ) INTERVAL_TYPE="$OPTARG";   ;;
        u ) USER="$OPTARG";            ;;
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
        killQuerysUser
    else
        logging
        killQuerys
    fi
fi

echo $(date)" -- End" >> $LOGFILE

{% from "components/greenplum/map.jinja" import gpdbvars with context %}
#!/bin/sh
conn=0
TIME=3600
HELP=0
USER=""

################################################################################################
# Objective  : To identify the CONNECTIONS  `<IDLE> in transaction` for more than 1 hour
# Summary    : This will identify the pid's and will issues a kill for then:
# Logging    : All logs will be stored in $HOME/gpAdminLogs/gp_idle.log
# Consequences  :
#            Connections killed will get the below error messages and they would need to reconnect
#            FATAL:  terminating connection due to administrator command
#            server closed the connection unexpectedly
#                This probably means the server terminated abnormally
#                before or while processing the request.
#            The connection to the server was lost. Attempting reset: Succeeded.
################################################################################################

help() {
    echo
   'Usage:
    kill_idle_in_transaction.sh [OPTION]
    -h    : Display help
    -c    : kill `<IDLE> in transaction` connections
    -t    : age of query and connection (indicated in seconds)
    -u    : ignoring user query

    By default, the -t option is 3600 seconds.
    Examples:

    kill_idle_in_transaction.sh -c  -t 3000 [kill connections `<IDLE> in transaction` for more then 3000s]
    kill_idle_in_transaction.sh -c  -u bob [kill connections `<IDLE> in transaction` for more then 3600s, ignoring bob user query]
    kill_idle_in_transaction.sh -c [kill connections `<IDLE> in transaction` for more then 3600s ]'
}

source ${HOME}/.bashrc
LOGFILE={{ gpdbvars.gplog }}/mvideo_kill_idle_in_transaction.log

#
# Function to log the `<IDLE> in transaction` sessions on the system
#
logging (){
    # Logging all the connections which were `<IDLE> in transaction` for more than 3600s
    # if query_start is not null then the user ran a query at some stage. So check the query_start time rather than the backend_start.
    psql -t -c "select * from pg_stat_activity where query_start is not null and state like 'idle in transaction%' and now()-query_start > '${TIME}s'" template1 >> $LOGFILE
}

#
# Functions to kill the `<IDLE> in transaction`
#
idle_conn () {
    # Generating pid's for connections `<IDLE> in transaction` opened for more then 1 hour:
    # if query_start is not null then the user ran a query at some stage. So check the query_start time rather than the backend_start.
    psql -A -t -c "SELECT 'kill '||pid from pg_stat_activity where query_start is not null and now()-query_start > '${TIME}s' and state like 'idle in transaction%' " template1 | bash
}

idle_conn_u () {
    # Generating pid's for connections `<IDLE> in transaction` opened for more then 1 hour:
    # This function excludes query from the user specified in the -u option
    psql -A -t -c "SELECT 'kill '||pid from pg_stat_activity where query_start is not null and now()-query_start > '${TIME}s' and state like 'idle in transaction%' and usename NOT IN (${USER})" template1 | bash
}


#
# Main program
#
if [ $# -ne 0 ] ; then
    while getopts "ct:u:h" opts ; do
        case  "$opts" in
            c)
                conn=1
            ;;
            t)
                TIME=$OPTARG;
            ;;
            h)
                HELP=1
            ;;
            u)
                USER="$OPTARG";
            ;;
        esac
    done
fi

echo $(date)" -- Start" >> $LOGFILE

if [ $HELP -eq 1 ] || [ $conn -eq 0 ];then
    help
fi

if [ $conn -eq 1 ];then
    if [ -n "$USER" ];then
        logging
        idle_conn_u
    else
        logging
        idle_conn
    fi
fi


echo $(date)" -- End" >> $LOGFILE

#!/bin/sh
script=`basename $0`

OPERATION=`echo $script | sed 's/sky_\(.*\).sh/\1/'`

HOSTS_FILE="hosts.txt";
PROGRAM_FILE=$OPERATION;
PROGRAM_PATH="~/bin";
MUST_CLEANUP="no";
NUM_ATTEMPTS=100;

while getopts ":ch:n:m:" opt; do
    case $opt in
    c)
        MUST_CLEANUP="yes";   
        ;;
    h)
        HOSTS_FILE=$OPTARG;
        ;;
    n)
        NUM_ATTEMPTS=$OPTARG;
        ;;
    m)
        PROGRAM_FILE=`basename $OPTARG`;
        PROGRAM_PATH=`dirname $OPTARG`;
        ;;
    \?)
        echo "Invalid option: -$OPTARG" >&2
        ;;
    esac
done

shift $((OPTIND-1))

if [ -z $2 ]; then
    echo "Usage: $script [options] <iters_number> <features_file> [features_test_file]"
    echo "Options:"
    echo "-c                            Do cleanup on hosts before start"
    echo "-h <hosts_file>               Provide hosts file (default: hosts.txt)"
    echo "-n <num_attempts>             Select number of attempts (default: 100)"
    echo "-m <${OPERATION}_executable>     Provide ${OPERATION} executable (default: ~/bin/${OPERATION})"
    echo "Example: $script -h hosts.txt -m ~/bin/${OPERATION} 3200 features.txt"
    exit 1
fi

NUM_ITERATIONS=$1
FEATURES_FILE=$2
FEATURES_TEST_FILE=$3

RAND_OFFSET=`jot -r 1 0 100`
MATNET_PORT=$((13701 + $RAND_OFFSET))
WORK_DIR_PREFIX=/var/tmp/`whoami`
WORK_DIR=${WORK_DIR_PREFIX}/${RAND_OFFSET}

if [ "$MUST_CLEANUP" = "yes" ]; then
    echo Cleanup
    sky run --hosts=$HOSTS_FILE "ps -awwx | grep ${WORK_DIR_PREFIX} | grep -E -v '(grep|python)' | awk '{print \$1}' | xargs kill; rm -rf ${WORK_DIR_PREFIX}/*"
fi

if [ .$OPERATION = .matrixnet ]; then
    MORE_PARAMS="-+ -s 10 -H ${HOSTS_FILE}"
fi

echo "Hosts: ${HOSTS_FILE}, prog: ${PROGRAM_FILE}, path: ${PROGRAM_PATH},  op: ${OPERATION}, wd: ${WORK_DIR}";
echo Prepare environment

sky run --hosts=$HOSTS_FILE "mkdir -p $WORK_DIR"
sky upload --hosts=$HOSTS_FILE -d $PROGRAM_PATH $PROGRAM_FILE $WORK_DIR/

stop_niggers()
{
    echo Stop
    echo Please wait until all niggers are terminated.
    sky run --hosts=$HOSTS_FILE "pkill -f ^$WORK_DIR/$PROGRAM_FILE"
    if [ -z $1 ]; then
        exit 1
    fi
}

trap stop_niggers SIGINT

ATTEMPT=0;

while [ ! -f ${OPERATION}.inc  -a  $ATTEMPT -lt $NUM_ATTEMPTS ]; do
    ATTEMPT=$(($ATTEMPT+1))
    echo Attempt $ATTEMPT
    echo Launch niggers
    sky run --hosts=$HOSTS_FILE --no_wait "$WORK_DIR/$PROGRAM_FILE -N -p $MATNET_PORT"
    sleep 1
    echo Run
    if [ -z $FEATURES_TEST_FILE ]; then
        nice ${PROGRAM_PATH}/${PROGRAM_FILE} -M -p $MATNET_PORT -f $FEATURES_FILE -i $NUM_ITERATIONS -c $MORE_PARAMS
    else
        nice ${PROGRAM_PATH}/${PROGRAM_FILE} -M -p $MATNET_PORT -f $FEATURES_FILE -t $FEATURES_TEST_FILE -i $NUM_ITERATIONS -c $MORE_PARAMS
    fi
done
 
stop_niggers do_not_exit

if [ ! -f ${OPERATION}.inc ]; then
    echo Maximum number of attempts exceeded
    exit 1;
fi


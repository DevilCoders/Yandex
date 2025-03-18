#!/bin/bash
set -e

export LANG=en_US.UTF8
export LANGUAGE=en_US.UTF8
export LC_ALL=en_US.UTF8
export LC_MESSAGES="en_US.UTF8"

POD_OPTS=""
if [[ ! -z "${DEPLOY_PROJECT_ID}" ]]; then

  # Configure CPU count
  # Exclude 400 millicores for logging
  CPU_COUNT=$(curl -s localhost:1/pod_attributes | \
    python -c "import sys, json; print max(1, int((json.load(sys.stdin)['resource_requirements']['cpu']['cpu_limit_millicores'] - 400) / 1000))")
  echo "Cpu count: ${CPU_COUNT}"
  POD_OPTS="-XX:ActiveProcessorCount=${CPU_COUNT}"

  # Configure RAM
  # Exclude 10% RAM for anon memory
  # Exclude 512 MiB RAM for logging
  RAM_SIZE=$(curl -s localhost:1/pod_attributes | \
    python -c "import sys, json; mem_mib = int(json.load(sys.stdin)['resource_requirements']['memory']['memory_guarantee_bytes'] / (1024 * 1024)); print max(512, mem_mib - int(mem_mib * 0.1) - 512)")
  echo "RAM size: ${RAM_SIZE}"
  POD_OPTS="${POD_OPTS} -XX:MaxRAM=${RAM_SIZE}m -XX:MaxRAMPercentage=75"
fi

APP_DIR=$(cd $(dirname `dirname "${BASH_SOURCE}[0]"`) && pwd -P)
BIN_DIR=${APP_DIR}/bin
LOG_DIR=/logs
CONFIG_DIR=${APP_DIR}/conf
HEAP_DUMP_DIR=/hprof

ENVIRONMENT=development
DEBUG_OPTS=""


#Read properties from script keys
for opt in "$@"; do
  case ${opt} in
    --environment=*) ENVIRONMENT="${opt#*=}"
    shift ;;
    --logdir=*) LOG_DIR="${opt#*=}"
    shift ;;
    --heapdump=*) HEAP_DUMP_DIR="${opt#*=}"
    shift ;;
    --debug=*) DEBUG_OPTS="${opt#*=}"
    shift ;;
    --java-home=*) JAVA_HOME="${opt#*=}"
    shift ;;
    *)
    ;;
  esac
done

mkdir -p ${LOG_DIR}
mkdir -p ${HEAP_DUMP_DIR}


if [[ -z "${JAVA_HOME}" ]] ; then
  JAVA_HOME="${APP_DIR}/jdk";
fi
echo "JAVA_HOME=${JAVA_HOME}"


CLASSPATH="${CONFIG_DIR}/:{% for item in CLASSPATH.split(PATHSEP) %}$APP_DIR/lib/{{item}}{% if not loop.last %}{{PATHSEP}}{% endif %}{% endfor %}"

GC_LOG_FILE="${LOG_DIR}/{{appName}}.gc.log"
GC_LOGGING_OPTIONS="-Xlog:gc*:file=${GC_LOG_FILE}:time:filecount=7,filesize=42M"

# TODO CI-2398
JDK17_HACKS="--add-opens=java.base/java.util=ALL-UNNAMED --add-opens=java.base/java.lang=ALL-UNNAMED --add-opens=java.base/java.time=ALL-UNNAMED"

echo "Classpath: $CLASSPATH"

JAVA_VERSION="{{JAVA_VERSION}}"
JAVA_MAJOR_VERSION="{{JAVA_MAJOR_VERSION}}"

echo "Java version ${JAVA_MAJOR_VERSION} (${JAVA_VERSION})"

if [[ "${ENVIRONMENT}" == "testing" ]] ; then
    MAIN_LOG_PERIOD="14d"
else
    MAIN_LOG_PERIOD="30d"
fi

exec -a {{appName}} ${JAVA_HOME}/bin/java -classpath "${CLASSPATH}" \
     -showversion -server -Xverify:none \
     ${GC_LOGGING_OPTIONS} \
     ${JDK17_HACKS} \
     -XX:ErrorFile="${LOG_DIR}/{{appName}}.hs_err.log" \
     -XX:+HeapDumpOnOutOfMemoryError \
     -XX:+ExitOnOutOfMemoryError \
     -XX:HeapDumpPath="${HEAP_DUMP_DIR}/" \
     -XX:OnOutOfMemoryError="chmod go+r ${HEAP_DUMP_DIR}/" \
     -XX:-OmitStackTraceInFastThrow \
     -Dfile.encoding=UTF-8 \
     -Dapp.name="{{appName}}" \
     -Dspring.profiles.active="${ENVIRONMENT}" \
     -Dlog.dir="${LOG_DIR}" \
     -Djava.net.preferIPv6Addresses=true \
     -Djava.net.preferIPv4Stack=false \
     -Dlog4j2.configurationFile="{{log4jConfigurations}}" \
     -Dorg.springframework.boot.logging.LoggingSystem=none \
     -DMAIN_LOG_PERIOD=${MAIN_LOG_PERIOD} \
     ${POD_OPTS} \
     {{jvmArgs}} \
     ${DEBUG_OPTS} \
     "{{mainClass}}"


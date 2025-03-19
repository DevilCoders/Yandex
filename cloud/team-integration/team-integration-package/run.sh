#!/bin/bash
set -xue

JAVA_HOME="${JAVA_HOME:-/usr}"
"${JAVA_HOME}/bin/java" --show-version ${JAVA_OPTS} -jar "$(dirname $0)/../lib/yc-team-integration.jar"

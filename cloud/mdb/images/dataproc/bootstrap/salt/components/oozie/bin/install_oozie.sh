#!/bin/bash
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

log_msg() {
    dt="$(date -u +'%d.%m.%y %H:%M:%S.%3N %Z')"
    level=$1
    if [ "${level}" = "DEBUG" -a "${YDP_DEBUG}" != "1" ] ; then
        return
    fi
    msg=${@:2}
    printf "${dt} ${level} ${msg}\n"
    if [ "${level}" = "FATAL" ] ; then
        cleanup && quit 1
    fi
}

set -ex
export PATH=/usr/bin:$PATH

OOZIE_DIR=/usr/lib/oozie
TMPDIR=$(mktemp -d)

cleanup() {
    rm -rf ${TMPDIR} || true
}

tar -xvzf ${OOZIE_DIR}/oozie-sharelib.tar.gz -C ${TMPDIR} || log_msg ERROR "Can't extract shared libraries"

# Fix conflict of versions jackson libraries
rm -f ${TMPDIR}/lib/hive2/jackson-*
cp /usr/lib/hadoop/lib/jackson-* ${TMPDIR}/share/lib/hive2/

# Upload oozie shared libs
hadoop fs -put -d -t 4 -f ${TMPDIR}/share /user/oozie/share || log_msg ERROR "Can't put oozie shared libraries to hdfs"
cleanup

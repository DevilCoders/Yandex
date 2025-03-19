# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

export ZEPPELIN_PORT=8890
export ZEPPELIN_CONF_DIR=/etc/zeppelin/conf
export ZEPPELIN_LOG_DIR=/var/log/zeppelin
export ZEPPELIN_PID_DIR=/var/run/zeppelin
export ZEPPELIN_WAR_TEMPDIR=/var/run/zeppelin/webapps
export ZEPPELIN_NOTEBOOK_DIR=/var/lib/zeppelin/notebook
export MASTER=yarn
export SPARK_HOME=/usr/lib/spark
export HADOOP_CONF_DIR=/etc/hadoop/conf
export HBASE_HOME=/usr/lib/hbase
export HBASE_CONF_DIR=/etc/hbase/conf

export ZEPPELIN_MEM=" "
export ZEPPELIN_JAVA_OPTS="${ZEPPELIN_JAVA_OPTS} -Dspark.executor.memory= "

# Use python from conda
export PYSPARK_PYTHON="/opt/conda/bin/python"
export PYSPARK_DRIVER_PYTHON="/opt/conda/bin/python"

# export SPARK_SUBMIT_OPTIONS="$SPARK_SUBMIT_OPTIONS --conf 'spark.executorEnv.PYTHONPATH=/usr/lib/spark/python/lib/py4j-src.zip:/usr/lib/spark/python/:<CPS>{{PWD}}/pyspark.zip<CPS>{{PWD}}/py4j-src.zip' --conf spark.yarn.isPython=true"

#!/usr/bin/env bash
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

export SPARK_HOME=${SPARK_HOME:-/usr/lib/spark}
export SPARK_LOG_DIR=${SPARK_LOG_DIR:-/var/log/spark}

export HADOOP_HOME=${HADOOP_HOME:-/usr/lib/hadoop}
export HADOOP_HDFS_HOME=${HADOOP_HDFS_HOME:-${HADOOP_HOME}/../hadoop-hdfs}
export HADOOP_MAPRED_HOME=${HADOOP_MAPRED_HOME:-${HADOOP_HOME}/../hadoop-mapreduce}
export HADOOP_YARN_HOME=${HADOOP_YARN_HOME:-${HADOOP_HOME}/../hadoop-yarn}
export HADOOP_CONF_DIR=${HADOOP_CONF_DIR:-/etc/hadoop/conf}

# Let's run everything with JVM runtime, instead of Scala
export SPARK_LAUNCH_WITH_SCALA=0
export SPARK_LIBRARY_PATH=${SPARK_LIBRARY_PATH:-${SPARK_HOME}/lib}
export SCALA_LIBRARY_PATH=${SCALA_LIBRARY_PATH:-${SPARK_HOME}/lib}

export SPARK_DIST_CLASSPATH=$(hadoop classpath)
export SPARK_LIBRARY_PATH=$SPARK_LIBRARY_PATH:${HADOOP_HOME}/lib/native
{% set spark_history_log_dir = salt['ydp-fs.fs_url_for_path'](
    '/var/log/spark/apps',
    allow_local=True,
) -%}
export SPARK_HISTORY_OPTS="$SPARK_HISTORY_OPTS -Dspark.history.fs.logDirectory={{ spark_history_log_dir }} -Dspark.history.ui.port=18080"

# Enable native libraries
export SPARK_LIBRARY_PATH="${SPARK_LIBRARY_PATH}:${HADOOP_HOME}/lib/native"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${HADOOP_HOME}/lib/native"

# Load java classpath on scala
export SPARK_SUBMIT_OPTS="${SPARK_SUBMIT_OPTS} -Dscala.usejavacp=true"
export SPARK_DAEMON_MEMORY="{{ salt['ydputils.get_spark_driver_mem_mb']() }}m"

# Load jdbc libraries
export SPARK_DIST_CLASSPATH="${SPARK_DIST_CLASSPATH}:/usr/share/java/mysql.jar:/usr/share/java/postgresql-jdbc4.jar:/usr/lib/clickhouse-jdbc-connector/clickhouse-jdbc.jar"

# Load hive depenencies
export SPARK_DIST_CLASSPATH="${SPARK_DIST_CLASSPATH}:/etc/hive/conf:/usr/lib/hive/lib/hive-spark-client.jar"

# Load Python from conda environment
export PYSPARK_PYTHON=${PYSPARK_PYTHON:-/opt/conda/bin/python}
export PYSPARK_DRIVER_PYTHON=${PYSPARK_DRIVER_PYTHON:-${PYSPARK_PYTHON}}

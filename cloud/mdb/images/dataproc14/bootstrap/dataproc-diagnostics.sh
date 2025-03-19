#!/usr/bin/env bash

VERSION="0.1"
OUTPUT_DIR=$(mktemp -d)
ARCHIVE_PATH=diagnostics.tgz
DPKG_VERIFY=${DPKG_VERIFY:-"true"}

echo "Creating an archive $ARCHIVE_PATH with system diagnostics info"

mkdir -p "$OUTPUT_DIR/configs"
mkdir -p "$OUTPUT_DIR/logs"
mkdir -p "$OUTPUT_DIR/java_traces"

echo "Collecting diagnostics collection script version"
echo $VERSION &> "$OUTPUT_DIR/diagnostics_version.txt"
sha256sum "$0" &> "$OUTPUT_DIR/diagnostics_hashsum.txt"

echo "Collecting uptime information"
uptime &> "$OUTPUT_DIR/uptime.txt"

echo "Collecting kernel version"
uname -a &> "$OUTPUT_DIR/kernel_version.txt"

echo "Collecting LSB information"
lsb_release --all &> "$OUTPUT_DIR/lsb.txt"

echo "Collecting lists of installed deb packages"
dpkg -l &> "$OUTPUT_DIR/installed_deb_packages.txt"

if [[ "${DPKG_VERIFY}" == "true" ]]; then
    echo "Collecting information on packages files integrity"
    dpkg --verify | cut -f3 -d' ' | xargs sha256sum &> "$OUTPUT_DIR/installed_deb_packages_integrity.txt"
fi

echo "Collecting status of supervised tasks"
supervisorctl status all &> "$OUTPUT_DIR/supervisor_status.txt"

echo "Collecting information about active sockets"
ss -nltp &> "$OUTPUT_DIR/active_sockets.txt"

echo "Collecting status of Ubuntu services"
service --status-all &> "$OUTPUT_DIR/services.txt"

echo "Collecting list of opened files"
lsof &> "$OUTPUT_DIR/opened_files.txt"

echo "Collecting list of running processes"
ps aux &> "$OUTPUT_DIR/processes.txt"

echo "Collecting mount points"
mount -v &> "$OUTPUT_DIR/mounts.txt"

echo "Collecting Java thread traces"
if command -v jps &> /dev/null; then
  for pid in $(jps -q); do
    cat "/proc/${pid}/cmdline" &> "$OUTPUT_DIR/java_traces/$pid"
    USER=$(ps u --pid "${pid}" | tail -n +2 | cut -f1 -d' ')
    sudo -H -u "${USER}" jstack "${pid}" &> "$OUTPUT_DIR/java_traces/$pid"
  done
fi

if command -v hdfs &> /dev/null; then
    echo "Collecting HDFS version"
    hdfs version &> "$OUTPUT_DIR/hdfs_version.txt"

    echo "Collecting HDFS configuration"
    hdfs getconf -namenodes &> "$OUTPUT_DIR/hdfs_name_nodes.txt"
    hdfs getconf -secondaryNameNodes &> "$OUTPUT_DIR/hdfs_secondary_name_nodes.txt"
    hdfs getconf -backupNodes &> "$OUTPUT_DIR/hdfs_backup_nodes.txt"
    hdfs getconf -nnRpcAddresses &> "$OUTPUT_DIR/hdfs_name_node_rpc_addresses.txt"
    hdfs storagepolicies -listPolicies &> "$OUTPUT_DIR/hdfs_storage_policies.txt"
fi

if command -v yarn &> /dev/null ; then
    echo "Collecting YARN version"
    yarn version &> "$OUTPUT_DIR/yarn_version.txt"

    echo "Collecting YARN nodes list"
    yarn node -all -list -showDetails &> "$OUTPUT_DIR/yarn_nodes.txt"

    echo "Collecting YARN application list"
    yarn application -list -appStates ALL &> "$OUTPUT_DIR/yarn_apps.txt"
fi

if [ -d /srv/pillar ]; then
    echo "Collecting Saltstack pillar"
    cp -R -L /srv/pillar "$OUTPUT_DIR/configs/"
fi

if [ -f /var/log/yandex-dataproc-bootstrap.log ]; then
    echo "Collecting dataproc bootstrap log"
    cp -R -L /var/log/yandex-dataproc-bootstrap.log "$OUTPUT_DIR/logs/"
fi

if [ -d /var/log/supervisor ]; then
    echo "Collecting supervisor logs"
    cp -R -L /var/log/supervisor "$OUTPUT_DIR/logs/"
fi

if [ -d /etc/tez ]; then
    echo "Collecting TEZ configs"
    cp -R -L /etc/tez "$OUTPUT_DIR/configs/"
fi

if [ -d /etc/zeppelin ]; then
    echo "Collecting Zeppelin configs"
    cp -R -L /etc/zeppelin "$OUTPUT_DIR/configs/"

    if [ -d /var/log/zeppelin ]; then
        echo "Collecting Zeppelin logs"
        cp -R -L /var/log/zeppelin "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/zookeeper ]; then
    echo "Collecting Zookeeper configs"
    cp -R -L /etc/zookeeper "$OUTPUT_DIR/configs/"

    if [ -d /var/log/zookeeper ]; then
        echo "Collecting Zookeeper logs"
        cp -R -L /var/log/zookeeper "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/sqoop ]; then
    echo "Collecting Sqoop configs"
    cp -R -L /etc/sqoop "$OUTPUT_DIR/configs/"

    if [ -d /var/log/sqoop ]; then
        echo "Collecting Sqoop logs"
        cp -R -L /var/log/sqoop "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/oozie ]; then
    echo "Collecting Oozie configs"
    cp -R -L /etc/oozie "$OUTPUT_DIR/configs/"

    if [ -d /var/log/oozie ]; then
        echo "Collecting Oozie logs"
        cp -R -L /var/log/oozie "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/hadoop ]; then
    echo "Collecting Hadoop configs"
    cp -R -L /etc/hadoop "$OUTPUT_DIR/configs/"

    if [ -d /var/log/hadoop-hdfs ]; then
        echo "Collecting HDFS logs"
        cp -R -L /var/log/hadoop-hdfs "$OUTPUT_DIR/logs/"
    fi
    if [ -d /var/log/hadoop-yarn ]; then
        echo "Collecting YARN logs"
        cp -R -L /var/log/hadoop-yarn "$OUTPUT_DIR/logs/"
    fi
    if [ -d /var/log/hadoop-mapreduce ]; then
        echo "Collecting Mapreduce logs"
        cp -R -L /var/log/hadoop-mapreduce "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/hbase ]; then
    echo "Collecting HBase configs"
    cp -R -L /etc/hbase "$OUTPUT_DIR/configs/"

    if [ -d /var/log/hbase ]; then
        echo "Collecting HBase logs"
        cp -R -L /var/log/hbase "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/flume ]; then
    echo "Collecting Flume configs"
    cp -R -L /etc/flume "$OUTPUT_DIR/configs/"

    if [ -d /var/log/flume ]; then
        echo "Collecting Flume logs"
        cp -R -L /var/log/flume "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/spark ]; then
    echo "Collecting Spark configs"
    cp -R -L /etc/spark "$OUTPUT_DIR/configs/"

    if [ -d /var/log/spark ]; then
        echo "Collecting Spark logs"
        cp -R -L /var/log/spark "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/postgresql ]; then
    echo "Collecting PostgreSQL configs"
    cp -R -L /etc/postgresql "$OUTPUT_DIR/configs/"
    if [ -d /var/log/postgresql-common ]; then
        cp -R -L /etc/postgresql-common "$OUTPUT_DIR/configs/"
    fi

    if [ -d /var/log/postgresql ]; then
        echo "Collecting PostgreSQL logs"
        cp -R -L /var/log/postgresql "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/salt ]; then
    echo "Collecting Salt configs"
    cp -R -L /etc/salt "$OUTPUT_DIR/configs/"

    if [ -d /var/log/salt ]; then
        echo "Collecting Salt logs"
        cp -R -L /var/log/salt "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/hive ]; then
    echo "Collecting Hive configs"
    cp -R -L /etc/hive "$OUTPUT_DIR/configs/"

    if [ -d /etc/hive-hcatalog ]; then
        cp -R -L /etc/hive-hcatalog "$OUTPUT_DIR/configs/"
    fi

    if [ -d /etc/hive-webhcat ]; then
        cp -R -L /etc/hive-webhcat "$OUTPUT_DIR/configs/"
    fi

    if [ -d /var/log/hive ]; then
        echo "Collecting Hive logs"
        cp -R -L /var/log/hive "$OUTPUT_DIR/logs/"
    fi
    if [ -d /var/log/hive-hcatalog ]; then
        cp -R -L /var/log/hive-hcatalog "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/knox ]; then
    echo "Collecting Knox configs"
    cp -R -L /etc/knox "$OUTPUT_DIR/configs/"

    if [ -d /var/log/knox ]; then
        echo "Collecting Knox logs"
        cp -R -L /var/log/knox "$OUTPUT_DIR/logs/"
    fi
fi

if [ -d /etc/livy ]; then
    echo "Collecting Livy configs"
    cp -R -L /etc/livy "$OUTPUT_DIR/configs/"

    if [ -d /var/log/livy ]; then
        echo "Collecting Livy logs"
        cp -R -L /var/log/livy "$OUTPUT_DIR/logs/"
    fi
fi

echo "Packing all collected information to the archive $ARCHIVE_PATH"
tar -C "$OUTPUT_DIR" -czf $ARCHIVE_PATH .

echo "Removing temporary files: $OUTPUT_DIR"
rm -rf "$OUTPUT_DIR"

echo "Please send the archive with diagnostics info $ARCHIVE_PATH to the Yandex.Cloud support along with the description of your problem"
echo "Note that config files are collected to the archive."
echo "If you added any kind of secrets such as passwords or tokens to the config files, make sure to remove it in archive before sending."

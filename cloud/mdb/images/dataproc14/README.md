# Managed Hadoop
This is a repository for working with bootstrap.
Directory bootstrap contains code and scripts for bootstrap node on any environment (virtual machine or docker container).
Directory vm-image contains a couple of scripts for building inital vm images based on Ubuntu Xenial.

# Build tasks
Build base image
https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocBaseImage
Build public Data Proc image
https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocImage
Move public Data Proc image to preprod public-images folder
https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocImageMove
Move public Data Proc image to prod public-images folder
https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=MDB_DataprocImageMoveToPreprod

# The List of TODO:
## Bootstrap
* Acquire lock on action
* Use logger instead of own solution
* Remove userdata-pillar after cold bootstrap
* Support JVM with Shanedoah https://wiki.openjdk.java.net/display/shenandoah/Main
* Support arguments and --help option
* Support signal handlers
* Support custom repositories for CI builds
* Extend filesystem to entire block device
  parted /dev/vda print "resizepart 1" Yes 100% print
* Fill /etc/managed-hadoop
  * version of image
  * cluster_id
  * components on cluster
  * components on current node
  * components versions ?
* wait-started.py (Waitting all services after install)
 * status of systemd services
 * maintanance mode ?
 * check ports
 * check some of metrics ?
 * blocking timeout with rechecks

## VM building
* Copy bootstrap on hadoop.image step, not on base.img
* Jenkins job for creating nightly docker image
  * Create using docker export / import
  * Run tests ?
  * Push to docker registry
  * Push to s3://s3.mdb.yandex.net/dbaas-images
* Jenkins job for creating nightly compute images
  * Run tests ?
  * Push to s3
* version scheme:
  * yc-hadoop-<branch>-<major.minor.fix>
    yc-hadoop-stable-2-0-1 Hadoop 2 with first fix
    yc-hadoop-stable-3-1-2 Hadoop 3 with minor updates and two fixes

    yc-hadoop-nightly-18_11_03 Latest image builded today
    yc-hadoop-trunk-0_<arcadia_revision>_<sandbox_task_id>

## Salt
* Forward s3 credentials https://github.com/apache/hadoop/blob/branch-2/hadoop-tools/hadoop-aws/src/site/markdown/tools/hadoop-aws/index.md
* Infrastructure
  * Running highstate on updated user-data. How to do it? Versions on user-data?
* Devoleper Tools
  * Docker
  * TensorFlow
  * python2, python3
* Security
  * HTTPS Certificates
  * Block Security
  * Internal Network Connectivity Security
* LDAP
* Kerberos
* Flume (on data / compute nodes) with configs in ZooKeeper
* HDFS
  * HA (+ add journal node)
    * Create hdfs directories only on active master
  * Create directory /user/{username} per every user
  * Salt modules instead of terrible cmd.runs?
  * Multiple subclusters of datanodes for heterogeneous disk types? https://hadoop.apache.org/docs/r2.7.3/hadoop-project-dist/hadoop-hdfs/ArchivalStorage.html#Configuration
  * Datanode balancing
* Hive
  * Change derby to postgres
  * Support MDB as a custom Metastore for HA
  * Support YandexDB as a store ?
  * LLAP
* YARN
  * Docker support (How to test it inside docker) ?
  * Run Web Application Proxy https://hadoop.apache.org/docs/current/hadoop-yarn/hadoop-yarn-site/WebApplicationProxy.html
* Sqoop
  * sqoop tools on all nodes
  * sqoop-metastore on masternodes with HA, replication and failover ?
* Rearrange order of applying site properties. Run now components << pillar << other-components.
  Should be components << other-components << pillar
* States for changing flavor. Change VM options, Heap sizes, thread count and others
* Depends component services on pillar hadoop-properties

## Building Packages
* Move all stuff to this repository
* Build all packages to release/master
* Build nightly packages to release/nightly
* Create thing for moving some packages to release/<version>
* Toolchain for working with s3 (through aptly)

## Daemon
* Watchdog
* Health
* Monitoring
* Decomission for YARN & HDFS http://hadoop.apache.org/docs/r2.9.1/hadoop-yarn/hadoop-yarn-site/GracefulDecommission.html
* Updating new ssh-keys ?

## Open Questions
* How to change system settings after resizing VM's without restarts?
* Support preemtible nodes?

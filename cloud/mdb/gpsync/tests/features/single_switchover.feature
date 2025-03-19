Feature: Single switchover with disabled rewind

    @single_switchover
    Scenario: Standby is promoted when switchover is intiated in ZK cluster
        Given a common started gpsync-controlled greenplum cluster
        When we lock "/gpsync/greenplum/switchover/lock" in zookeeper "zookeeper1"
        And we set value "{"hostname": "gpsync-greenplum-master1.gpsync.gpsync.net","timeline": 1}" for key "/gpsync/greenplum/switchover/master" in zookeeper "zookeeper1"
        And we set value "scheduled" for key "/gpsync/greenplum/switchover/state" in zookeeper "zookeeper1"
        And we release lock "/gpsync/greenplum/switchover/lock" in zookeeper "zookeeper1"
        Then container "greenplum_master2" became a master
        And pgbouncer is running in container "greenplum_master2"
        And greenplum master in container "greenplum_master1" is not running
        And pgbouncer is not running in container "greenplum_master1"
        And greenplum in container "greenplum_master1" was not rewinded

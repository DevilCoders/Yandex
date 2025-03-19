Feature: Single failover with disabled rewind

    @single_failover
    Scenario Outline: Standby is promoted when primary master fails
        Given a common started gpsync-controlled greenplum cluster
        When we <destroy> container "greenplum_master1"
        Then container "greenplum_master2" became a master
        And pgbouncer is running in container "greenplum_master2"
        When we <repair> container "greenplum_master1" and <start gpsync>
        Then greenplum master in container "greenplum_master1" is not running
        And pgbouncer is not running in container "greenplum_master1"
        And greenplum in container "greenplum_master1" was not rewinded
    
    Examples: <destroy>, <repair>, <start gpsync>
            | destroy                 | repair             | start gpsync |
            | stop                    | start              | start gpsync |
            | disconnect from network | connect to network | do nothing   |

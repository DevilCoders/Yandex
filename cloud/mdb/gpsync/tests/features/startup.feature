Feature: Gpsync startup

  @startup
  Scenario: Gpsync launches alongside with greenplum cluster and does not break it
    Given a following greenplum cluster
    """
      greenplum_master1:
        role: master_primary
      greenplum_master2:
        role: master_standby
      greenplum_segment1:
        role: segment
      greenplum_segment2:
        role: segment
    """
    When we start "greenplum" in container "greenplum_master1"
    And we start "greenplum" in container "greenplum_master2"
    Then greenplum master in container "greenplum_master1" is up and reports all segments are also up
    And container "greenplum_master2" is a replica of container "greenplum_master1"
    When we start "gpsync" in container "greenplum_master1"
    And we start "gpsync" in container "greenplum_master2"
    Then zookeeper "zookeeper1" has holder "gpsync-greenplum-master1.gpsync.gpsync.net" for lock "/gpsync/greenplum/leader"
    And zookeeper "zookeeper1" has following values for key "/gpsync/greenplum/replics_info"
    """
      - client_hostname: gpsync-greenplum-master2.gpsync.gpsync.net
        state: streaming
    """
    And container "greenplum_master2" is in sync group
    And pgbouncer is running in container "greenplum_master1"
    But pgbouncer is not running in container "greenplum_master2"

  @startup
  Scenario: Gpsync launches greenplum master on primary and standby
    Given a following greenplum cluster
    """
      greenplum_master1:
        role: master_primary
      greenplum_master2:
        role: master_standby
      greenplum_segment1:
        role: segment
      greenplum_segment2:
        role: segment
    """
    When we start "gpsync" in container "greenplum_master1"
    And we start "gpsync" in container "greenplum_master2"
    Then greenplum master in container "greenplum_master1" is up and reports all segments are also up
    And container "greenplum_master2" is a replica of container "greenplum_master1"
    Then zookeeper "zookeeper1" has holder "gpsync-greenplum-master1.gpsync.gpsync.net" for lock "/gpsync/greenplum/leader"
    And zookeeper "zookeeper1" has following values for key "/gpsync/greenplum/replics_info"
    """
      - client_hostname: gpsync-greenplum-master2.gpsync.gpsync.net
        state: streaming
    """
    And container "greenplum_master2" is in sync group
    And pgbouncer is running in container "greenplum_master1"
    But pgbouncer is not running in container "greenplum_master2"

Feature: Gpsync manages dbid for master segment

  @dbids
  Scenario: Gpsync publishes current dbid of master segments and gpsync on standby node uses it
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
    Then zookeeper "zookeeper1" has following values for key "/gpsync/greenplum/dbid_info" sorted by "hostname"
    """
      - dbid: 1
        role: p
        hostname: gpsync-greenplum-master1.gpsync.gpsync.net
      - dbid: 6
        role: m
        hostname: gpsync-greenplum-master2.gpsync.gpsync.net
    """
    When we run following command on host "greenplum_master2"
    """
    rm -f /var/lib/greenplum/data1/master/gpseg-1/internal.auto.conf
    """
    When we start "gpsync" in container "greenplum_master2"
    Then container "greenplum_master2" is in sync group 

  @dbids
  Scenario: Gpsync refuses to start standby master without published dbid, then requests its creation lauches standby master once dbid is created
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
    When we start "gpsync" in container "greenplum_master2"
    And we wait "10.0" seconds
    Then greenplum master in container "greenplum_master2" is not running
    When we start "gpsync" in container "greenplum_master1"
    Then container "greenplum_master2" is in sync group
    When we gracefully stop "gpsync" in container "greenplum_master2"
    And we gracefully stop "greenplum" in container "greenplum_master2"
    And we remove standby master from greenplum cluster in container "greenplum_master1"
    Then zookeeper "zookeeper1" has following values for key "/gpsync/greenplum/dbid_info" sorted by "hostname"
    """
      - dbid: 1
        role: p
        hostname: gpsync-greenplum-master1.gpsync.gpsync.net
    """
    When we gracefully stop "gpsync" in container "greenplum_master1"
    And we gracefully stop "greenplum" in container "greenplum_master1"
    When we start "gpsync" in container "greenplum_master2"
    Then zookeeper "zookeeper1" has value "gpsync-greenplum-master2.gpsync.gpsync.net" for key "/gpsync/greenplum/create_dbid_for_host"
    When we wait "10.0" seconds
    Then greenplum master in container "greenplum_master2" is not running
    When we start "gpsync" in container "greenplum_master1"
    Then zookeeper "zookeeper1" has value "None" for key "/gpsync/greenplum/create_dbid_for_host"
    And zookeeper "zookeeper1" has following values for key "/gpsync/greenplum/dbid_info" sorted by "hostname"
    """
      - dbid: 1
        role: p
        hostname: gpsync-greenplum-master1.gpsync.gpsync.net
      - dbid: 6
        role: m
        hostname: gpsync-greenplum-master2.gpsync.gpsync.net
    """
    And container "greenplum_master2" is in sync group

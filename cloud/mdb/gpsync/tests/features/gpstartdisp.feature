Feature: Test gpstartdisp script.
         We check that it is capable of starting both primary master instance and standby master instance.

  @gpstartdisp
  Scenario: Start greenplum cluster's master nodes independently
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
    Then greenplum master in container "greenplum_master1" is up and reports all segments are also up
    But greenplum master in container "greenplum_master2" is not running
    When we start "greenplum" in container "greenplum_master2"
    Then greenplum master in container "greenplum_master2" is recovering
    And container "greenplum_master2" is a replica of container "greenplum_master1"

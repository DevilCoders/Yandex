Feature: Check WAL archiving works correctly

    @archiving
    Scenario: Check that archive enabled after restart postgres without maintenance
        Given a "gpsync" container common config
        """
        postgresql.conf:
            archive_mode: on
            archive_command: '/bin/true'
        """
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
        Then zookeeper "zookeeper1" has holder "gpsync-greenplum-master1.gpsync.gpsync.net" for lock "/gpsync/greenplum/leader"
        And greenplum master in container "greenplum_master1" has value "/bin/true" for option "archive_command"
        And container "greenplum_master1" has following config
        """
        postgresql.auto.conf: {}
        """
        When we set value "/bin/false" for option "archive_command" in "postgresql.auto.conf" config in container "greenplum_master1"
        Then greenplum master in container "greenplum_master1" has value "/bin/true" for option "archive_command"
        And container "greenplum_master1" has following config
        """
        postgresql.auto.conf: {}
        """

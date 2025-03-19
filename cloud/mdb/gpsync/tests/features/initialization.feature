Feature: Test that tests infrastructure works correctly.
         We need to check that all system starts and works
         as expected as is, without any intervention.

    @init
    Scenario: Start containers with preconfigured greenplum and launch GP
        Given a "gpsync" container common config
        """
            postgresql.conf:
                archive_mode: on
                archive_command: 'echo "%p" && rsync --contimeout=1 --timeout=1 -a --password-file=/etc/archive.passwd %p rsync://archive@gpsync-backup1.gpsync.gpsync.net:/archive/segment-1/%f'
        """
        Given a "zookeeper" container "zookeeper1"
        And a "zookeeper" container "zookeeper2"
        And a "zookeeper" container "zookeeper3"
        And a "backup" container "backup1"
        And a "greenplum" container "greenplum_segment1"
        And a "greenplum" container "greenplum_segment2"
        And a "gpsync" container "greenplum_master1"
        And a "gpsync" container "greenplum_master2" with following config
        """
            recovery.conf:
                restore_command: 'rsync -a --password-file=/etc/archive.passwd rsync://archive@gpsync-backup1.gpsync.gpsync.net:/archive/segment-1/%f %p'
        """
        Then container "greenplum_master1" has following config
        """
            postgresql.conf:
                archive_mode: on
                archive_command: 'echo "%p" && rsync --contimeout=1 --timeout=1 -a --password-file=/etc/archive.passwd %p rsync://archive@gpsync-backup1.gpsync.gpsync.net:/archive/segment-1/%f'
        """
        And container "greenplum_master2" has following config
        """
            postgresql.conf:
                archive_mode: on
                archive_command: 'echo "%p" && rsync --contimeout=1 --timeout=1 -a --password-file=/etc/archive.passwd %p rsync://archive@gpsync-backup1.gpsync.gpsync.net:/archive/segment-1/%f'
            recovery.conf:
                restore_command: 'rsync -a --password-file=/etc/archive.passwd rsync://archive@gpsync-backup1.gpsync.gpsync.net:/archive/segment-1/%f %p'
        """
        When we start "greenplum" in container "greenplum_master1"
        Then greenplum master in container "greenplum_master1" is up and reports all segments are also up

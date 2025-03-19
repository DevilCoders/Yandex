Feature: Check not HA hosts

    @failover
    Scenario Outline: Check not ha host from master
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'no'
                    postgres_timeout: 5
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    recovery_timeout: 5
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql1_1.pgsync_pgsync_net
                stream_from: postgresql1
        """

        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        When we disconnect from network container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        When we connect to network container "postgresql1"
        Then container "postgresql3" is a replica of container "postgresql1"
        And container "postgresql1" is in <replication_type> group

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |      yes      |      quorum      |
        | zookeeper | zookeeper1 |      no       |       sync       |

        @failover
    Scenario Outline: Check cascade replica
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'no'
                    postgres_timeout: 5
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    recovery_timeout: 5
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql2_1.pgsync_pgsync_net
                stream_from: postgresql2
        """

        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in <replication_type> group
        Then container "postgresql3" is a replica of container "postgresql2"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        When we disconnect from network container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        When we connect to network container "postgresql1"
        Then container "postgresql1" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |      yes      |      quorum      |
        | zookeeper | zookeeper1 |      no       |       sync       |


    @auto_stream_from
    Scenario Outline: Cascade replica streams from master when replication source fails
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    recovery_timeout: 30
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql2_1.pgsync_pgsync_net
                stream_from: postgresql2
        """

        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in quorum group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        When we disconnect from network container "postgresql2"
        Then container "postgresql3" is a replica of container "postgresql1"
        When we connect to network container "postgresql2"
        Then container "postgresql3" is a replica of container "postgresql2"

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @auto_stream_from
    Scenario Outline: Cascade replica streams from new master when old master fails and it is replication source
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    recovery_timeout: 30
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql1_1.pgsync_pgsync_net
                stream_from: postgresql1
        """

        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in quorum group
        And <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        And container "postgresql3" is a replica of container "postgresql1"
        When we disconnect from network container "postgresql1"
        Then container "postgresql2" became a master
        And container "postgresql3" is a replica of container "postgresql2"
        When we connect to network container "postgresql1"
        Then container "postgresql1" is a replica of container "postgresql2"
        And container "postgresql3" is a replica of container "postgresql1"

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |


    @auto_stream_from
    Scenario Outline: Cascade replica waits new master if there are no hosts for streaming in HA
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    recovery_timeout: 30
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
            postgresql.conf:
                wal_sender_timeout: '2s'
                wal_receiver_timeout: '2s'
        """
        Given a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql1_1.pgsync_pgsync_net
                stream_from: postgresql1
        """

        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        When we remember postgresql start time in container "postgresql2"
        When we disconnect from network container "postgresql1"
        And we wait "10.0" seconds
        When we connect to network container "postgresql1"
        Then postgresql in container "postgresql2" was not restarted
        And postgresql in container "postgresql2" was not rewinded
        Then container "postgresql2" is a replica of container "postgresql1"

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |



    @auto_stream_from
    Scenario Outline: Cascade replica returns stream from replication source if it is cascade replica too
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    quorum_commit: 'yes'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    recovery_timeout: 30
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_without_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql2_1.pgsync_pgsync_net
                stream_from: postgresql2
            postgresql4:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            stream_from: pgsync_postgresql3_1.pgsync_pgsync_net
                stream_from: postgresql3
        """

        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql2" is in quorum group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        When we disconnect from network container "postgresql3"
        Then container "postgresql4" is a replica of container "postgresql1"
        When we connect to network container "postgresql3"
        Then container "postgresql4" is a replica of container "postgresql3"

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |
        | zookeeper | zookeeper1 |

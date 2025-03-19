Feature: Check that replicas change master consecutively

    @switchover
    Scenario Outline: Change consecutively on failover
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: '<use_slots>'
                    do_consecutive_remaster: 'yes'
                    election_timeout: 10
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
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_<with_slots>_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" <with_slots> replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 4
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 3
            postgresql4:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 2
            postgresql5:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then container "postgresql2" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql4_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql5_1.pgsync_pgsync_net
            state: streaming
        """
        When we stop container "postgresql1"
        Then container "postgresql2" became a master
        Then "3" containers are replicas of "postgresql2" within "120.0" seconds
        And at least "3" postgresql instances were running simultaneously during test
        Then postgresql in container "postgresql3" was not rewinded
        Then postgresql in container "postgresql4" was not rewinded
        Then postgresql in container "postgresql5" was not rewinded

    Examples: <lock_type>, <with_slots> replication slots
        | lock_type | lock_host  | with_slots | use_slots | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 |   with     |    yes    |      no       |       sync       |

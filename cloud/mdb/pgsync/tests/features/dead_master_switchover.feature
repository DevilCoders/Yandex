Feature: Switchover with dead master

    @switchover
    Scenario Outline: Check successful switchover with dead master
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    autofailover: '<autofailover>'
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 3
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 3
                    min_failover_timeout: 120
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
                config:
                    pgsync.conf:
                        global:
                            priority: 3
            postgresql2:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 2

        """
        Then container "postgresql3" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        When we disconnect from network container "postgresql1"
        And we make switchover task with params "<destination>" in container "postgresql2"
        Then container "<new_master>" became a master
        And container "<replica>" is a replica of container "<new_master>"
        Then postgresql in container "<replica>" was not rewinded
        When we connect to network container "postgresql1"
        Then container "<new_master>" is master
        And container "postgresql1" is a replica of container "<new_master>"
        And container "postgresql1" is in <replication_type> group
        And postgresql in container "<replica>" was not rewinded
        And postgresql in container "postgresql1" was rewinded

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  |                   destination             | new_master  | replica     | autofailover | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |                      None                 | postgresql3 | postgresql2 |      no      |      yes      |      quorum      |
        | zookeeper | zookeeper1 | -d pgsync_postgresql2_1.pgsync_pgsync_net | postgresql2 | postgresql3 |      no      |      yes      |      quorum      |
        | zookeeper | zookeeper1 |                      None                 | postgresql3 | postgresql2 |      yes     |      yes      |      quorum      |
        | zookeeper | zookeeper1 | -d pgsync_postgresql2_1.pgsync_pgsync_net | postgresql3 | postgresql2 |      yes     |      yes      |      quorum      |
        | zookeeper | zookeeper1 |                      None                 | postgresql3 | postgresql2 |      no      |      no       |       sync       |
        | zookeeper | zookeeper1 | -d pgsync_postgresql2_1.pgsync_pgsync_net | postgresql2 | postgresql3 |      no      |      no       |       sync       |
        | zookeeper | zookeeper1 |                      None                 | postgresql3 | postgresql2 |      yes     |      no       |       sync       |
        | zookeeper | zookeeper1 | -d pgsync_postgresql2_1.pgsync_pgsync_net | postgresql3 | postgresql2 |      yes     |      no       |       sync       |

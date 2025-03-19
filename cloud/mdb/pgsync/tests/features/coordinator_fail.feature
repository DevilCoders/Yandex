Feature: Check availability on coordinator failure

    @coordinator_fail
    Scenario Outline: Kill coordinator
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: '<use_slots>'
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
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        When we disconnect from network container "zookeeper1"
        And we disconnect from network container "zookeeper2"
        And we disconnect from network container "zookeeper3"
        And we wait "10.0" seconds
        Then pgbouncer is running in container "postgresql1"
        And pgbouncer is running in container "postgresql2"
        And pgbouncer is running in container "postgresql3"

    Examples: <lock_type>, <with_slots> slots
        | lock_type | lock_host  | with_slots | use_slots | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 |   with     |    yes    |      no       |       sync       |

    @coordinator_fail
    Scenario Outline: Kill coordinator and both replicas
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: '<use_slots>'
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
                            priority: 2
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 1
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        When we disconnect from network container "zookeeper1"
        And we disconnect from network container "zookeeper2"
        And we disconnect from network container "zookeeper3"
        And we disconnect from network container "postgresql2"
        And we disconnect from network container "postgresql3"
        And we wait "10.0" seconds
        Then pgbouncer is not running in container "postgresql1"

    Examples: <lock_type>, <with_slots> slots
        | lock_type | lock_host  | with_slots | use_slots | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 |   with     |    yes    |      no       |       sync       |

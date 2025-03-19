Feature: Destroy new master after promote and before sync with zookeeper

    @failed_promote
    Scenario Outline: New master will continue to be master after restart during promote
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
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_<with_slots>_slot.sh %m %p
                debug:
                    promote_checkpoint_sql: CHECKPOINT; SELECT pg_sleep('infinity');
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
        When we <destroy> container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        When we stop container "postgresql2"
        When we start container "postgresql2"
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
        Then container "postgresql3" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        Then postgresql in container "postgresql3" was not rewinded
        When we <repair> container "postgresql1"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql1" is a replica of container "postgresql2"
        Then pgsync in container "postgresql1" is connected to zookeeper
        Then postgresql in container "postgresql1" was rewinded

    Examples: <lock_type>, <replication_type> replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      no       |       sync       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      no       |       sync       |


    @failed_promote_return_master
    Scenario Outline: New master will continue to be master after returning old master during restart in promote section
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
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_<with_slots>_slot.sh %m %p
                debug:
                    promote_checkpoint_sql: CHECKPOINT; SELECT pg_sleep('infinity');
        """
        Given a following cluster with "<lock_type>" <with_slots> replication slots
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
        When we <destroy> container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        When we stop container "postgresql2"
        When we <repair> container "postgresql1"
        When we start container "postgresql2"
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
        Then container "postgresql1" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        Then container "postgresql1" is a replica of container "postgresql2"
        Then pgsync in container "postgresql1" is connected to zookeeper
        Then postgresql in container "postgresql3" was not rewinded
        Then postgresql in container "postgresql1" was rewinded

    Examples: <lock_type>, <replication_type> replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      no       |       sync       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      no       |       sync       |

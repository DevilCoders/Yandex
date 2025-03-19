Feature: Destroy master in various scenarios


    @failover
    Scenario Outline: Destroy master on 2-hosts cluster with remaster_restart = <remaster_restart>
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: '<use_slots>'
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
                    remaster_restart: <remaster_restart>
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
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" is in quorum group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        When we <destroy> container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
    Examples: <lock_type>, synchronous replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | remaster_restart |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |        no        |


    @failover
    Scenario Outline: Destroy master one by one with remaster_restart = <remaster_restart>
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: '<use_slots>'
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
                    remaster_restart: <remaster_restart>
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
        Then container "postgresql2" is in quorum group
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
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
        Then container "postgresql3" is in quorum group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        Then postgresql in container "postgresql3" was not rewinded
        Then <lock_type> "<lock_host>" has value "["pgsync_postgresql3_1.pgsync_pgsync_net"]" for key "/pgsync/postgresql/quorum"
        When we <destroy> container "postgresql2"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql3" became a master
    Examples: <lock_type>, synchronous replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | quorum_commit | replication_type | remaster_restart |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      yes      |      quorum      |        no        |


    @failover
    Scenario Outline: Destroy master with remaster_restart = <remaster_restart>
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
                    remaster_restart: <remaster_restart>
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
        When we <destroy> container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
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

    Examples: <lock_type>, synchronous replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | quorum_commit | replication_type | remaster_restart |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      yes      |      quorum      |        yes       |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      yes      |      quorum      |        yes       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      yes      |      quorum      |        yes       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      yes      |      quorum      |        yes       |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      no       |       sync       |        yes       |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      no       |       sync       |        yes       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      no       |       sync       |        yes       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      no       |       sync       |        yes       |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      yes      |      quorum      |        no        |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      yes      |      quorum      |        no        |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      yes      |      quorum      |        no        |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      yes      |      quorum      |        no        |
        | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |      no       |       sync       |        no        |
        | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |      no       |       sync       |        no        |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      no       |       sync       |        no        |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      no       |       sync       |        no        |


    @failover
    Scenario Outline: Destroy master with async replication
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: '<use_slots>'
                master:
                    change_replication_type: '<change_replication>'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: '<data_loss>'
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
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/sync_replica"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
            sync_state: <sync_state>
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
            sync_state: async
        """
        When we <destroy> container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/sync_replica"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
            sync_state: async
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        Then postgresql in container "postgresql3" was not rewinded
        When we <repair> container "postgresql1"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
            sync_state: <sync_state>
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
            sync_state: async
        """
        Then container "postgresql1" is a replica of container "postgresql2"
        Then pgsync in container "postgresql1" is connected to zookeeper
        Then postgresql in container "postgresql1" was rewinded

        Examples: <lock_type>, <sync_state>hronous replication <with_slots> slots, <destroy>/<repair>
            | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | sync_state | change_replication | data_loss |
            | zookeeper | zookeeper1 |           stop          |        start       |  without   |    no     |    async   |        no          |    yes    |
            | zookeeper | zookeeper1 |           stop          |        start       |   with     |    yes    |    async   |        no          |    yes    |
            | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |    async   |        no          |    yes    |
            | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |    async   |        no          |    yes    |


    @failover_archive
    Scenario Outline: Destroy master with one replica in archive recovery
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
                    remaster_checks: 3
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
                    remaster_restart: 'no'
                    recovery_timeout: 20
                commands:
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
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
        Then container "postgresql2" is in quorum group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        When we stop container "postgresql3"
        When we drop replication slot "pgsync_postgresql3_1_pgsync_pgsync_net" in container "postgresql1"
        When we start container "postgresql3"
        Then <lock_type> "<lock_host>" has value "["pgsync_postgresql2_1.pgsync_pgsync_net"]" for key "/pgsync/postgresql/quorum"
        When we wait "10.0" seconds
        When we <destroy> container "postgresql1"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql2_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql2" became a master
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
        Then container "postgresql3" is in quorum group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
        """
        Then container "postgresql3" is a replica of container "postgresql2"
        Then postgresql in container "postgresql3" was not rewinded
        When we <repair> container "postgresql1"
        Then container "postgresql1" is a replica of container "postgresql2"
        Then pgsync in container "postgresql1" is connected to zookeeper
        Then postgresql in container "postgresql1" was rewinded
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
        """
    Examples: <lock_type>, synchronous replication with slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |
        | zookeeper | zookeeper1 |           stop          |        start       |

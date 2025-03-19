Feature: Testing min_failover_timeout setting

    @failover
    Scenario Outline: Destroy master and wait min_failover_timeout seconds
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
                    min_failover_timeout: 240
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
        Then postgresql in container "postgresql1" was rewinded
        When we <destroy> container "postgresql2"
        Then <lock_type> "<lock_host>" has holder "None" for lock "/pgsync/postgresql/leader"
        Then container "postgresql3" is in <replication_type> group
        When we wait until "10.0" seconds to failover of "postgresql3" left in <lock_type> "<lock_host>"
        Then <lock_type> "<lock_host>" has holder "None" for lock "/pgsync/postgresql/leader"
        Then container "postgresql3" is in <replication_type> group
        When we wait "10.0" seconds
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        Then container "postgresql3" became a master
        Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
        Then container "postgresql1" is in <replication_type> group
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
        """
        When we <repair> container "postgresql2"
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """

    Examples: <lock_type>, <sync_state>hronous replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots | quorum_commit | replication_type |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      yes      |      quorum      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      yes      |      quorum      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |      no       |       sync       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |      no       |       sync       |


    Scenario Outline: Destroy master and wait min_failover_timeout seconds with async replication
      Given a "pgsync" container common config
          """
              pgsync.conf:
                  global:
                      priority: 0
                      use_replication_slots: '<use_slots>'
                  master:
                      change_replication_type: 'no'
                      remaster_checks: 1
                  replica:
                      allow_potential_data_loss: 'yes'
                      master_unavailability_timeout: 1
                      remaster_checks: 1
                      min_failover_timeout: 240
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
              sync_state: async
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
              sync_state: async
            - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
              state: streaming
              sync_state: async
          """
      Then container "postgresql1" is a replica of container "postgresql2"
      Then postgresql in container "postgresql1" was rewinded
      When we <destroy> container "postgresql2"
      Then <lock_type> "<lock_host>" has holder "None" for lock "/pgsync/postgresql/leader"
      Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/sync_replica"
      When we wait until "10.0" seconds to failover of "postgresql3" left in <lock_type> "<lock_host>"
      Then <lock_type> "<lock_host>" has holder "None" for lock "/pgsync/postgresql/leader"
      Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/sync_replica"
      When we wait "10.0" seconds
      Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
      Then container "postgresql3" became a master
      Then <lock_type> "<lock_host>" has value "finished" for key "/pgsync/postgresql/failover_state"
      Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/sync_replica"
      Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
          """
            - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
              state: streaming
              sync_state: async
          """
      When we <repair> container "postgresql2"
      Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
          """
            - client_hostname: pgsync_postgresql1_1.pgsync_pgsync_net
              state: streaming
              sync_state: async
            - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
              state: streaming
              sync_state: async
          """

      Examples: <lock_type>, <sync_state>hronous replication <with_slots> slots, <destroy>/<repair>
        | lock_type | lock_host  |          destroy        |       repair       | with_slots | use_slots |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |  without   |    no     |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |   with     |    yes    |

Feature: Replication slots

    @slots
    Scenario Outline: Slots created on promoted replica
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                master:
                    change_replication_type: 'no'
                    remaster_checks: 1
                replica:
                    allow_potential_data_loss: 'yes'
                    master_unavailability_timeout: 1
                    remaster_checks: 1
                    min_failover_timeout: 1
                    master_unavailability_timeout: 2
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
        Then <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
            sync_state: async
            write_location_diff: 0
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
            sync_state: async
            write_location_diff: 0
        """
        Then container "postgresql1" has following replication slots
        """
          - slot_name: pgsync_postgresql2_1_pgsync_pgsync_net
            slot_type: physical
          - slot_name: pgsync_postgresql3_1_pgsync_pgsync_net
            slot_type: physical
        """
        Then container "postgresql2" has following replication slots
        """
        """
        Then container "postgresql3" has following replication slots
        """
        """
        When we stop container "postgresql1"
        Then container "postgresql2" became a master
        Then container "postgresql2" has following replication slots
        """
          - slot_name: pgsync_postgresql1_1_pgsync_pgsync_net
            slot_type: physical
          - slot_name: pgsync_postgresql3_1_pgsync_pgsync_net
            slot_type: physical
        """

    Examples: <lock_type>
        |   lock_type   |   lock_host    |
        |   zookeeper   |   zookeeper1   |

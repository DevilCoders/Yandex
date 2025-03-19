Feature: Check plugins

    Scenario Outline: Check upload_wals plugin
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
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
                            priority: 1
            postgresql3:
                role: replica
                config:
                    pgsync.conf:
                        global:
                            priority: 2
        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql3_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/sync_replica"
        When we disable archiving in "postgresql1"
        And we switch wal in "postgresql1" "10" times
        And we stop container "postgresql1"
        Then container "postgresql3" became a master
        And wals present on backup "<backup_host>"
    Examples: <lock_type>, <lock_host>
        | lock_type | backup_host  | lock_host |
        | zookeeper | backup1 | zookeeper1     |

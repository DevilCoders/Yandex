Feature: Check pgsync with disabled autofailover
    @switchover_test
    Scenario Outline: Check switchover with disabled autofailover
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    autofailover: 'no'
                    quorum_commit: '<quorum_commit>'
                master:
                    change_replication_type: 'yes'
                    remaster_checks: 3
                replica:
                    allow_potential_data_loss: 'no'
                    master_unavailability_timeout: 1
                    remaster_checks: 3
                    min_failover_timeout: 60
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
                            priority: 2
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
                            priority: 3

        """
        Then container "postgresql3" is in <replication_type> group
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 1}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then container "postgresql3" became a master
        And container "postgresql2" is a replica of container "postgresql3"
        And container "postgresql1" is a replica of container "postgresql3"
        Then postgresql in container "postgresql2" was not rewinded
        Then postgresql in container "postgresql1" was not rewinded
        Then container "postgresql1" is in <replication_type> group
        When we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql3_1.pgsync_pgsync_net","timeline": 2}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        Then container "postgresql1" became a master
        And container "postgresql3" is a replica of container "postgresql1"
        And container "postgresql2" is a replica of container "postgresql1"
        When we stop container "postgresql2"
        And we lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we set value "{"hostname": "pgsync_postgresql1_1.pgsync_pgsync_net","timeline": 3}" for key "/pgsync/postgresql/switchover/master" in <lock_type> "<lock_host>"
        And we set value "scheduled" for key "/pgsync/postgresql/switchover/state" in <lock_type> "<lock_host>"
        And we release lock "/pgsync/postgresql/switchover/lock" in <lock_type> "<lock_host>"
        And we wait "30.0" seconds
        Then container "postgresql1" is master
        When we wait "30.0" seconds
        Then container "postgresql3" became a master
        And container "postgresql1" is a replica of container "postgresql3"

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit | replication_type |
        | zookeeper | zookeeper1 |      yes      |      quorum      |
        | zookeeper | zookeeper1 |      no       |       sync       |

        @failover
    Scenario Outline: Check kill master with disabled autofailover
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    autofailover: 'no'
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
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
                config:
                    pgsync.conf:
                        global:
                            priority: 2
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
                            priority: 3

        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        When we <destroy> container "postgresql1"
        And we wait "30.0" seconds
        Then <lock_type> "<lock_host>" has holder "None" for lock "/pgsync/postgresql/leader"
        When we <repair> container "postgresql1"
        Then container "postgresql2" is a replica of container "postgresql1"
        And container "postgresql3" is a replica of container "postgresql1"

    Examples: <lock_type>, <lock_host>, <destroy>, <repair>
        | lock_type | lock_host  |          destroy        |       repair       | quorum_commit |
        | zookeeper | zookeeper1 |           stop          |        start       |      yes      |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |      yes      |
        | zookeeper | zookeeper1 |           stop          |        start       |      no       |
        | zookeeper | zookeeper1 | disconnect from network | connect to network |      no       |

    Scenario Outline: Check suddenly external promote replica
    We consider unexpected external promote as an error, so we leave old master as it is.
    Moreover, pgsync should switch off pgbouncer on suddenly promoted host to avoid split brain state.
        Given a "pgsync" container common config
        """
            pgsync.conf:
                global:
                    priority: 0
                    use_replication_slots: 'yes'
                    autofailover: 'no'
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
                    generate_recovery_conf: /usr/local/bin/gen_rec_conf_with_slot.sh %m %p
        """
        Given a following cluster with "<lock_type>" with replication slots
        """
            postgresql1:
                role: master
                config:
                    pgsync.conf:
                        global:
                            priority: 2
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
                            priority: 3

        """
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And <lock_type> "<lock_host>" has following values for key "/pgsync/postgresql/replics_info"
        """
          - client_hostname: pgsync_postgresql3_1.pgsync_pgsync_net
            state: streaming
          - client_hostname: pgsync_postgresql2_1.pgsync_pgsync_net
            state: streaming
        """
        When we promote host "postgresql2"
        Then <lock_type> "<lock_host>" has holder "pgsync_postgresql1_1.pgsync_pgsync_net" for lock "/pgsync/postgresql/leader"
        And container "postgresql1" is master
        And pgbouncer is not running in container "postgresql2"
        And pgbouncer is running in container "postgresql1"
        And pgbouncer is running in container "postgresql3"

    Examples: <lock_type>, <lock_host>
        | lock_type | lock_host  | quorum_commit |
        | zookeeper | zookeeper1 |      yes      |
        | zookeeper | zookeeper1 |      no       |

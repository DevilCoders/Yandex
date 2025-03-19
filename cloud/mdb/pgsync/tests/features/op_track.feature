Feature: Destructive operation tracking

    Scenario Outline: No lock on master if unfinished op is present
        Given a "pgsync" container common config
        """
            pgsync.conf:
                replica:
                    master_unavailability_timeout: 100500
        """
        And a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
        """
        When we set value "rewind" for key "/pgsync/postgresql/all_hosts/pgsync_postgresql1_1.pgsync_pgsync_net/op" in <lock_type> "<lock_host>"
        Then <lock_type> "<lock_host>" has holder "None" for lock "/pgsync/postgresql/leader"
        And pgbouncer is not running in container "postgresql1"

    Examples: <lock_type>
        |   lock_type   |   lock_host   |
        |   zookeeper   |   zookeeper1  |

    Scenario Outline: Unfinished op is properly cleaned up on replica
        Given a "pgsync" container common config
        """
            pgsync.conf:
                replica:
                    master_unavailability_timeout: 100500
        """
        And a following cluster with "<lock_type>" without replication slots
        """
            postgresql1:
                role: master
            postgresql2:
                role: replica
            postgresql3:
                role: replica
        """
        When we set value "rewind" for key "/pgsync/postgresql/all_hosts/pgsync_postgresql2_1.pgsync_pgsync_net/op" in <lock_type> "<lock_host>"
        Then <lock_type> "<lock_host>" has value "None" for key "/pgsync/postgresql/all_hosts/pgsync_postgresql2_1.pgsync_pgsync_net/op"

    Examples: <lock_type>
        |   lock_type   |   lock_host   |
        |   zookeeper   |   zookeeper1  |

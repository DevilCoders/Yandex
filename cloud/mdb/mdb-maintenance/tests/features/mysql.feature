Feature: MySQL maintenance tasks

    Scenario: mysql_5_7_update_tls_certs config works
        Given mysql cluster with "5.7.25" minor version and name "mysql_5_7_update_tls_certs"
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        When I successfully load config with name "mysql_5_7_update_tls_certs"
        Then cluster selection successfully returns
        """
        [cid1]
        """

    Scenario: mysql_8_0_update_tls_certs config works
        Given mysql cluster with "8.0.17" minor version and name "mysql_8_0_update_tls_certs"
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        When I successfully load config with name "mysql_8_0_update_tls_certs"
        Then cluster selection successfully returns
        """
        [cid1]
        """

    Scenario: mysql_update_minor_version config works
        Given mysql cluster with "8.0.1" minor version
        And move cluster "cid1" to "dev" env
        When I successfully load config with name "mysql_update_minor_version"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: MySQL mw task args contains zk_hosts
        Given mysql cluster with "8.0.1" minor version
        And move cluster "cid1" to "dev" env
        And "mysql_update_minor_version_dev" config
        """
        info: Minor update of MySQL
        clusters_selection:
         db: >
          SELECT cid
          FROM (
                   SELECT cid,
                          env,
                          type,
                          actual_rev,
                          'mysql' as component
                   FROM dbaas.clusters
                   WHERE status = 'RUNNING'
                     AND type = 'mysql_cluster'
                     AND env = 'dev'
               ) c
                   JOIN dbaas.versions USING (cid, component)
                   JOIN dbaas.default_versions
                        ON default_versions.type = c.type
                            AND default_versions.component = c.component
                            AND default_versions.env = c.env
                            AND default_versions.major_version = versions.major_version
                            AND default_versions.edition = versions.edition
                            AND default_versions.minor_version != versions.minor_version

        pillar_change: >
          UPDATE dbaas.versions p
          SET minor_version = default_v.minor_version,
              package_version = default_v.package_version
          FROM (
                   SELECT default_versions.minor_version,
                          default_versions.package_version
                   FROM (
                            SELECT *,
                                   'mysql' AS component
                            FROM dbaas.clusters
                            WHERE clusters.cid = :cid
                        ) c
                            JOIN dbaas.versions USING (cid, component)
                            JOIN dbaas.default_versions USING (component, env, major_version, edition)
               ) as default_v
          WHERE p.cid = :cid

        worker:
          operation_type: mysql_cluster_modify
          task_type: mysql_cluster_maintenance
          task_args:
            restart: true
            update_tls: true
          timeout: 24h
        """
        When I maintain "mysql_update_minor_version_dev" config
        Then there is one maintenance task
        And exists "mysql_update_minor_version_dev" maintenance task on "cid1" in "PLANNED" status
        And cluster "cid1" have "mysql_cluster_maintenance" task with argument "zk_hosts" equal to
        """
        ["localhost"]
        """

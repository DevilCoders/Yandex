Feature: PostgreSQL maintenance tasks

    Background: PostgreSQL cluster and MW task
        Given postgresql cluster with "10.1" minor version
        And move cluster "cid1" to "dev" env
        And stopped postgresql cluster with name "name"
        And move cluster "cid2" to "dev" env

    Scenario: Postgresql_offline_update_tls_certs config works
        Given set tls expirations of hosts of cluster "cid2" to "2000-01-01T00:00:00Z"
        And move cluster "cid2" to "prod" env
        When I successfully load config with name "postgresql_offline_update_tls_certs"
        Then cluster selection successfully returns
        """
        [cid2]
        """

    Scenario: Postgresql_fast_update_tls_certs config works
        Given set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        When I successfully load config with name "postgresql_fast_update_tls_certs"
        Then cluster selection successfully returns
        """
        [cid1]
        """

    Scenario: Postgresql_offline_update_minor_version config works
        When I successfully load config with name "postgresql_offline_update_minor_version"
        And I set "cid2" "postgres" version to "10.1"
        And move cluster "cid2" to "prod" env
        Then cluster selection successfully returns
        """
        [cid2]
        """
        When I successfully execute pillar_change on "cid2"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: Postgresql_fast_update_minor_version config works
        When I successfully load config with name "postgresql_fast_update_minor_version"
        And move cluster "cid1" to "dev" env
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

    Scenario: Postgresql mw task args contains zk_hosts
        Given "postgresql_update_minor_version_dev" config
        """
        info: Minor update of PostgreSQL
        clusters_selection:
         db: >
          SELECT cid
          FROM (
                   SELECT cid,
                          env,
                          type,
                          actual_rev,
                          'postgres' as component
                   FROM dbaas.clusters
                   WHERE status = 'RUNNING'
                     AND type = 'postgresql_cluster'
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
                                   'postgres' AS component
                            FROM dbaas.clusters
                            WHERE clusters.cid = :cid
                        ) c
                            JOIN dbaas.versions USING (cid, component)
                            JOIN dbaas.default_versions USING (component, env, major_version, edition)
               ) as default_v
          WHERE p.cid = :cid

        worker:
          operation_type: postgresql_cluster_modify
          task_type: postgresql_cluster_maintenance
          task_args:
            restart: true
            update_tls: true
          timeout: 24h
        """
        When I maintain "postgresql_update_minor_version_dev" config
        Then there is one maintenance task
        And exists "postgresql_update_minor_version_dev" maintenance task on "cid1" in "PLANNED" status
        And cluster "cid1" have "postgresql_cluster_maintenance" task with argument "zk_hosts" equal to
        """
        "localhost"
        """

   Scenario: postgresql_offline_update_odyssey_version config works
        When I successfully load config with name "postgresql_offline_update_odyssey_version"
        When I set "cid2" "odyssey" version to "0.322"
        And move cluster "cid2" to "prod" env
        Then cluster selection successfully returns
        """
        [cid2]
        """
        When I successfully execute pillar_change on "cid2"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: postgresql_fast_update_odyssey_version config works
        When I successfully load config with name "postgresql_fast_update_odyssey_version"
        When I set "cid1" "odyssey" version to "0.322"
        And move cluster "cid1" to "dev" env
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: postgresql_update_odyssey_version_hotfix config works
        Given I set "cid1" "odyssey" version to "Odyssey-2022"
        When I successfully load config with name "postgresql_update_odyssey_version_hotfix"
        Then cluster selection successfully returns
        """
        [cid1]
        """
        When I successfully execute pillar_change on "cid1"
        Then cluster selection successfully returns
        """
        []
        """

   Scenario: priority option works
        Given "some_mw_task" config
        """
        description: some_mw_task
        clusters_selection:
          db: >
              SELECT cid
                FROM dbaas.clusters c
                JOIN dbaas.pillar p
               USING (cid)
               WHERE type = 'postgresql_cluster'
                 AND coalesce((p.value->'data'->>'foo'), 'no') != 'yes'
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,foo}', to_jsonb(CAST('yes' AS text)))
           WHERE cid = :cid
        """
        And "priority_task" config
        """
        description: priority_task
        priority: 1
        clusters_selection:
          db: >
              SELECT cid
                FROM dbaas.clusters c
                JOIN dbaas.pillar p
               USING (cid)
               WHERE type = 'postgresql_cluster'
                 AND coalesce((p.value->'data'->>'bar'), 'no') != 'yes'
        pillar_change: >
          UPDATE dbaas.pillar p
             SET value = jsonb_set(value, '{data,bar}', to_jsonb(CAST('yes' AS text)))
           WHERE cid = :cid
        """
        When I maintain "some_mw_task" config
        Then there is one maintenance task
        And exists "some_mw_task" maintenance task on "cid1" in "PLANNED" status
        And I maintain "priority_task" config
        Then there are "2" maintenance tasks
        And exists "some_mw_task" maintenance task on "cid1" in "COMPLETED" status
        And exists "priority_task" maintenance task on "cid1" in "PLANNED" status

  Scenario: postgresql update tls config doesn't change edition
        When I load all configs
        And move cluster "cid1" to "prod" env
        And I set cluster "cid1" edition to "1c"
        And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
        When I maintain "postgresql_offline_update_tls_certs" config
        Then cluster "cid1" edition is equal to "1c"

  Scenario: postgresql fast update tls config doesn't change edition
      When I load all configs
      And I set cluster "cid1" edition to "1c"
      And set tls expirations of hosts of cluster "cid1" to "2000-01-01T00:00:00Z"
      When I maintain "postgresql_fast_update_tls_certs" config
      Then cluster "cid1" edition is equal to "1c"

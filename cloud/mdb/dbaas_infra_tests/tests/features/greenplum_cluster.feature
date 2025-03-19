@dependent-scenarios
Feature: Internal API Greenplum Cluster lifecycle

  Background: Wait until internal api is ready
    Given Go Internal API is up and running
    And actual MetaDB version
    And actual DeployDB version
    And Deploy API is up and running
    And Fake DBM is up and running
    And s3 is up and running
    And Mlock api is up and running
    And Secrets api is up and running
    And we are working with standard Greenplum cluster

  @setup
  Scenario: Greenplum cluster with standby master created successfully
    Given feature flag "MDB_GREENPLUM_CLUSTER" is set
    When we create "Greenplum" cluster "test_cluster" with following config overrides [grpc]
    """
    {
    'master_host_count': 2
    }
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "25 minutes" [grpc]
    And "Greenplum" cluster "test_cluster" is up and running [grpc]
    And cluster has no pending changes

  @password_change
  Scenario: Greenplum main user password change
    When we attempt to update "Greenplum" cluster "test_cluster" [grpc]
    """
    update_mask:
      paths:
        - user_password
    user_password: AnotherPa$$w0rd
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "25 minutes" [grpc]
    And we are able to log in to "Greenplum" cluster "test_cluster" using
    """
    password: AnotherPa$$w0rd
    """
    And cluster has no pending changes

  @backups
  Scenario: Backups
    When we get Greenplum backups for "test_cluster" [gRPC]
    Then backups count is greater than "0"
    And we remember current response as "initial_backups"
    When we restore Greenplum using latest "initial_backups" and config [grpc]
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    config:
        zone_id: sas
    master_resources:
        resourcePresetId: db1.nano
        diskSize: 10737418240
        diskTypeId: local-ssd
    segment_resources:
        resourcePresetId: db1.medium
        diskSize: 10737418240
        diskTypeId: local-ssd
    """
    Then response status is "OK" [grpc]
    And generated task is finished within "25 minutes" [grpc]
    And cluster has no pending changes

  @delete
  Scenario: Delete Greenplum cluster
    When we attempt to delete "Greenplum" cluster "test_cluster" [grpc]
    Then response status is "OK" [grpc]
    And generated task is finished within "5 minutes" [grpc]
    And in worker_queue exists "greenplum_cluster_delete_metadata" task
    And this task is done
    And we are unable to find "Greenplum" cluster "test_cluster" [grpc]
    But s3 has bucket for cluster
    And in worker_queue exists "greenplum_cluster_purge" task
    When delayed_until time has come
    Then this task is done
    And s3 has no bucket for cluster

Feature: Create Compute Greenplum cluster

  Background: Wait until go internal api is ready
    Given we are working with standard Greenplum cluster

  @create
  Scenario: Greenplum cluster create successfully
    When we try to create greenplum cluster "test_cluster" with following config overrides
    """
    {
      "segment_config": {
        "resources": {
          "disk_type_id": "local-ssd",
          "disk_size": 107374182400
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "15 minutes" via GRPC
    And greenplum cluster "test_cluster" is up and running

  # commented out because of ssh keys still hasn't been added to configuration.py
  #@alive-check
  #Scenario: Greenplum cluster is really alive and readable
  #  Given greenplum cluster "test_cluster" is up and running
  #  And following SQL request in Greenplum cluster "test_cluster" succeeds
  #  """
  #  select count(*) > 0 can_read from gp_dist_random('gp_id');
  #  """
  #  And query result is exactly
  #  """
  #  - can_read: True
  #  """

  @backups
  Scenario: Greenplum cluster should restore from backup successfully
    When we get Greenplum backups for "test_cluster" via GRPC
    Then backups count is greater than "0"
    And we remember current response as "initial_backups"
    When we restore Greenplum using latest "initial_backups" and config via GRPC
    """
    name: test_cluster_at_last_backup
    environment: PRESTABLE
    config:
        zone_id: ru-central1-a
    master_resources:
        resourcePresetId: s2.micro
        diskSize: 17179869184
        diskTypeId: network-ssd
    segment_resources:
        resourcePresetId: s2.micro
        diskSize: 107374182400
        diskTypeId: local-ssd
    """
    Then grpc response should have status OK
    And generated task is finished within "20 minutes" via GRPC
    And greenplum cluster "test_cluster_at_last_backup" is up and running

  @backups @delete
  Scenario: Greenplum cluster delete successfully
    Given greenplum cluster "test_cluster_at_last_backup" exists
    When we try to remove greenplum cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And in worker_queue exists "greenplum_cluster_delete_metadata" task
    And this task is done
    And in worker_queue exists "greenplum_cluster_purge" task
    When delayed_until time has come
    Then this task is done

  @modify-master
  Scenario: Greenplum master's flavor type update successfully
    Given greenplum cluster "test_cluster" is up and running
    When we try to update greenplum cluster
    """
    {
      "master_config": {
        "resources": {
          "resource_preset_id": "s2.small"
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "40 minutes" via GRPC
    And greenplum cluster "test_cluster" is up and running

  Scenario: Greenplum master's disk size update successfully
    Given greenplum cluster "test_cluster" is up and running
    When we try to update greenplum cluster
    """
    {
      "master_config": {
        "resources": {
          "disk_size": 21474836480
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "40 minutes" via GRPC
    And greenplum cluster "test_cluster" is up and running

  @modify-segment
  Scenario: Greenplum segment's flavor update successfully
    Given greenplum cluster "test_cluster" is up and running
    When we try to update greenplum cluster
    """
    {
      "segment_config": {
        "resources": {
          "resource_preset_id": "s2.small"
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "40 minutes" via GRPC
    And greenplum cluster "test_cluster" is up and running

  Scenario: Greenplum segment's disk size update successfully
    Given greenplum cluster "test_cluster" is up and running
    When we try to update greenplum cluster
    """
    {
      "segment_config": {
        "resources": {
          "disk_size": 214748364800
        }
      }
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "40 minutes" via GRPC
    And greenplum cluster "test_cluster" is up and running

  Scenario: Greenplum add segments
    Given greenplum cluster "test_cluster" is up and running
    When we try to add hosts to greenplum cluster
    """
    {
        "segment_host_count": 2
    }
    """
    Then grpc response should have status OK
    And generated task is finished within "40 minutes" via GRPC
    And greenplum cluster "test_cluster" is up and running

  @delete
  Scenario: Greenplum cluster delete successfully
    Given greenplum cluster "test_cluster" exists
    When we try to remove greenplum cluster
    Then grpc response should have status OK
    And generated task is finished within "5 minutes" via GRPC
    And in worker_queue exists "greenplum_cluster_delete_metadata" task
    And this task is done
    And in worker_queue exists "greenplum_cluster_purge" task
    When delayed_until time has come
    Then this task is done


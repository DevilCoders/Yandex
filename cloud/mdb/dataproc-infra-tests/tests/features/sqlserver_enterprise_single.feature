Feature: Create Compute SQLServer cluster

    Background: Wait until go internal api is ready
        Given we are working with enterprise SQLServer cluster

    @create
    Scenario: SQL Server EE creation works
        When we try to create sqlserver cluster "single_node_cluster" with following config overrides
        """
        { 
            "hostSpecs": [{
                "zoneId": "ru-central1-b",
                "subnetId": "bltnas5si736upc8ucq9"
            }]
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "30 minutes" via GRPC
        And sqlserver database "testdb" exists
        And sqlserver user "test" can connect to "testdb"
        When we add test data "foo" on cluster "single_node_cluster" in "testdb"
        Then test data "foo" exists on cluster "single_node_cluster" in "testdb"

    Scenario: Database management
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to create database "betadb"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver database "betadb" exists
        When we try to delete database "betadb"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver database "betadb" not exist

    Scenario: User management
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to create user "batman" with standard password and "DB_DDLADMIN" role in database "testdb"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver user "batman" can connect to "testdb"
        And sqlserver user "batman" can create table in "testdb"
        And sqlserver user "batman" can NOT select from "testdb" 
        When we try to grant "DB_DATAREADER" role in database "testdb" to user "batman"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver user "batman" can select from "testdb" 
        And sqlserver user "batman" can NOT insert into "testdb" 
        When we try to grant "DB_DATAWRITER" role in database "testdb" to user "batman"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver user "batman" can select from "testdb" 
        And sqlserver user "batman" can insert into "testdb" 
        When we try to revoke "DB_DATAWRITER" role in database "testdb" from user "batman"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver user "batman" can select from "testdb" 
        And sqlserver user "batman" can NOT insert into "testdb" 
        When we try to delete user "batman"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver user "batman" can NOT connect to "testdb"
        When we try to create user "batman" with standard password
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And sqlserver user "batman" can NOT connect to "testdb"

    @backup
    Scenario: SQL Server EE backup creation works
        Given sqlserver cluster "single_node_cluster" is up and running
        When we remember last cluster backup name
        When we create backup for selected sqlserver cluster via GRPC
        Then grpc response should have status OK
        And generated task is finished within "7 minutes" via GRPC
        Then last cluster backup name greater than saved

    @dbexportimport
    Scenario: Assign service account to SQL Server
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to update sqlserver cluster
        """
        {
          "service_account_id": "{{ serviceAccountId4 }}"
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "15 minutes" via GRPC

    @dbexportimport
    Scenario: Backup export and import works
        Given sqlserver cluster "single_node_cluster" is up and running
        And put random s3 bucket name to context under client_bucket_name key
        And s3 bucket with key client_bucket_name exists
        When we try to export sqlserver database backup
        """
        {
            "database_name": "testdb",
            "s3_bucket": "{{ client_bucket_name }}",
            "s3_path": "/testpath",
            "prefix": "testdb_copy"
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "5 minutes" via GRPC
        When we try to import sqlserver database backup
        """
        {
            "database_name": "testdb_reimported",
            "s3_bucket": "{{ client_bucket_name }}",
            "s3_path": "/testpath",
            "files": ["testdb_copy_000.bak"]
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "10 minutes" via GRPC
        When we try to grant "DB_OWNER" role in database "testdb_reimported" to user "test"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And test data "foo" exists on cluster "single_node_cluster" in "testdb_reimported"

    @dbrestore
    Scenario: Restore existing database
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to restore database "testdb" as "testdb_copy"
        Then grpc response should have status OK
        And generated task is finished within "10 minutes" via GRPC
        And sqlserver database "testdb_copy" exists
        When we try to grant "DB_OWNER" role in database "testdb_copy" to user "test"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And test data "foo" exists on cluster "single_node_cluster" in "testdb_copy"
        When we remember current time as "pitr1"
        And we wait for "5 seconds"
        And we try to delete database "testdb_copy"
        Then grpc response should have status OK
        And generated task is finished within "5 minutes" via GRPC
        When we try to restore database "testdb_copy" as "testdb_copy2" at "pitr1"
        Then grpc response should have status OK
        And generated task is finished within "10 minutes" via GRPC
        When we try to grant "DB_OWNER" role in database "testdb_copy2" to user "test"
        Then grpc response should have status OK
        And generated task is finished within "2 minutes" via GRPC
        And test data "foo" exists on cluster "single_node_cluster" in "testdb_copy2"

    @backup
    Scenario: Restoring SQL Server EE from backup to original folder works
        Given sqlserver cluster "single_node_cluster" is up and running
        When we restore sqlserver cluster from latest backup from cluster "single_node_cluster" via GRPC
        """
        name: restored_sqlserver
        environment: PRESTABLE
        configSpec:
          sqlserver_config_2016sp2ent:
             max_degree_of_parallelism: 0
        """
        Then grpc response should have status OK
        And generated task is finished within "30 minutes" via GRPC
        Given we are selecting "restored_sqlserver" sqlserver cluster for following steps
        Then test data "foo" exists on cluster "restored_sqlserver" in "testdb"
        When we try to remove SQLServer cluster
        Then grpc response should have status OK
        And generated task is finished within "5 minutes" via GRPC

    Scenario: scale flavor up
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to update sqlserver cluster
        """
        {
            "config_spec": {
                "resources": {
                    "resource_preset_id": "s2.medium"
                }
            }
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "30 minutes" via GRPC
        And test data "foo" exists on cluster "single_node_cluster" in "testdb"

    Scenario: scale flavor down
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to update sqlserver cluster
        """
        {
            "config_spec": {
                "resources": {
                    "resource_preset_id": "s2.small"
                }
            }
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "30 minutes" via GRPC
        And test data "foo" exists on cluster "single_node_cluster" in "testdb"

    Scenario: scale disk up
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to update sqlserver cluster
        """
        {
            "config_spec": {
                "resources": {
                    "disk_size": 21474836480
                }
            }
        }
        """
        Then grpc response should have status OK
        And generated task is finished within "30 minutes" via GRPC
        And test data "foo" exists on cluster "single_node_cluster" in "testdb"

    Scenario: scale disk down
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to update sqlserver cluster
        """
        {
            "config_spec": {
                "resources": {
                    "disk_size": 10737418240
                }
            }
        }
        """
        Then grpc response should fail with status PERMISSION_DENIED and body contains
        """
        requested feature is not available
        """

    @delete
    Scenario: Remove SQL Server EE cluster
        Given sqlserver cluster "single_node_cluster" is up and running
        When we try to remove SQLServer cluster
        Then grpc response should have status OK
        And generated task is finished within "5 minutes" via GRPC

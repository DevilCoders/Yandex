@elasticsearch
@grpc_api
Feature: Elasticsearch auth settings operations
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "user_specs": [{
            "name": "donn",
            "password": "nomanisanisland100500"
        }, {
            "name": "frost",
            "password": "2roadsd1v3rgedInAYelloWood"
        }],
        "config_spec": {
            "version": "7.14",
            "edition": "platinum",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we "AddProviders" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1",
        "providers": [{
          "type": "NATIVE",
          "name": "native1",
          "order": "1",
          "enabled": true,

          "hidden": false,
          "description": "Login",
          "hint": "login here",
          "icon": "logoElasticsearch"
        },
        {
          "type": "SAML",
          "name": "saml1",
          "enabled": true,

          "hidden": false,
          "description": "Login SAML",
          "hint": "login me please",
          "icon": "https://saml.com/my-logo.jpg",
          "saml": {
                "idp_entity_id": "https://test_identity_provider.com",
                "idp_metadata_file": "PEVudGl0eURlc2NyaXB0b3IgZW50aXR5SUQ9Imh0dHBzOi8vdGVzdF9pZGVudGl0eV9wcm92aWRlci5jb20iPjwvRW50aXR5RGVzY3JpcHRvcj4=",
                "sp_entity_id": "https://test_service_provider.com",
                "kibana_url": "https://test_kibana.com",
                "attribute_principal": "test_principal",
                "attribute_groups": "a",
                "attribute_name": "b",
                "attribute_email": "c",
                "attribute_dn": "d"
          }
        }]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add authentication providers to ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.AddAuthProvidersMetadata",
            "cluster_id": "cid1",
            "names": ["native1", "saml1"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker

  Scenario: Add saml auth provider
    When we "AddProviders" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1",
        "providers": [{
          "type": "SAML",
          "name": "saml2",
          "enabled": true,
          "order": "1",

          "hidden": false,
          "description": "Login SAML",
          "hint": "login me please",
          "icon": "https://saml.com/my-logo.jpg",
          "saml": {
                "idp_entity_id": "https://test_identity_provider.com",
                "idp_metadata_file": "PEVudGl0eURlc2NyaXB0b3IgZW50aXR5SUQ9Imh0dHBzOi8vdGVzdF9pZGVudGl0eV9wcm92aWRlci5jb20iPjwvRW50aXR5RGVzY3JpcHRvcj4=",
                "sp_entity_id": "https://test_service_provider.com",
                "kibana_url": "https://test_kibana.com",
                "attribute_principal": "test_principal",
                "attribute_groups": "a",
                "attribute_name": "b",
                "attribute_email": "c",
                "attribute_dn": "d"
          }
        }]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add authentication providers to ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.AddAuthProvidersMetadata",
            "cluster_id": "cid1",
            "names": ["saml2"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "GetProvider" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "saml2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "type": "SAML",
        "name": "saml2",
        "order": "1",
        "enabled": true,
        "hidden": false,
        "description": "Login SAML",
        "hint": "login me please",
        "icon": "https://saml.com/my-logo.jpg",
        "saml": {
            "idp_entity_id": "https://test_identity_provider.com",
            "idp_metadata_file": "PEVudGl0eURlc2NyaXB0b3IgZW50aXR5SUQ9Imh0dHBzOi8vdGVzdF9pZGVudGl0eV9wcm92aWRlci5jb20iPjwvRW50aXR5RGVzY3JpcHRvcj4=",
            "sp_entity_id": "https://test_service_provider.com",
            "kibana_url": "https://test_kibana.com",
            "attribute_principal": "test_principal",
            "attribute_groups": "a",
            "attribute_name": "b",
            "attribute_email": "c",
            "attribute_dn": "d"
        }
    }
    """

  Scenario: Update auth provider
    When we "UpdateProvider" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "saml1",
        "provider": {
          "type": "SAML",
          "name": "saml2",
          "enabled": true,
          "order": "1",

          "hidden": true,
          "description": "Login SAMLqqq",
          "hint": "login me please qqq",
          "icon": "my-logo.png",
          "saml": {
            "idp_entity_id": "https://test_identity_provider_2.com",
            "idp_metadata_file": "PEVudGl0eURlc2NyaXB0b3IgZW50aXR5SUQ9Imh0dHBzOi8vdGVzdF9pZGVudGl0eV9wcm92aWRlcl8yLmNvbSI+PC9FbnRpdHlEZXNjcmlwdG9yPg==",
            "sp_entity_id": "https://test_service_provider_2.com",
            "kibana_url": "https://test_kibana_2.com",
            "attribute_principal": "test_principal_2",
            "attribute_groups": "a2",
            "attribute_name": "b2",
            "attribute_email": "c2",
            "attribute_dn": "d2"
          }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update authentication providers in ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.UpdateAuthProvidersMetadata",
            "cluster_id": "cid1",
            "names": ["saml2"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "GetProvider" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "saml2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "type": "SAML",
        "name": "saml2",
        "order": "1",
        "enabled": true,
        "hidden": true,
        "description": "Login SAMLqqq",
        "hint": "login me please qqq",
        "icon": "my-logo.png",
        "saml": {
            "idp_entity_id": "https://test_identity_provider_2.com",
            "idp_metadata_file": "PEVudGl0eURlc2NyaXB0b3IgZW50aXR5SUQ9Imh0dHBzOi8vdGVzdF9pZGVudGl0eV9wcm92aWRlcl8yLmNvbSI+PC9FbnRpdHlEZXNjcmlwdG9yPg==",
            "sp_entity_id": "https://test_service_provider_2.com",
            "kibana_url": "https://test_kibana_2.com",
            "attribute_principal": "test_principal_2",
            "attribute_groups": "a2",
            "attribute_name": "b2",
            "attribute_email": "c2",
            "attribute_dn": "d2"
        }
    }
    """

  Scenario: Delete native provider
    When we "DeleteProvider" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "native1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete authentication providers from ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.DeleteAuthProvidersMetadata",
            "cluster_id": "cid1",
            "names": ["native1"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "ListProviders" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.AuthService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "providers": [
            {
                "type": "SAML",
                "name": "saml1",
                "order": "1",
                "enabled": true,
                "hidden": false,
                "description": "Login SAML",
                "hint": "login me please",
                "icon": "https://saml.com/my-logo.jpg",
                "saml": {
                    "idp_entity_id": "https://test_identity_provider.com",
                    "idp_metadata_file": "PEVudGl0eURlc2NyaXB0b3IgZW50aXR5SUQ9Imh0dHBzOi8vdGVzdF9pZGVudGl0eV9wcm92aWRlci5jb20iPjwvRW50aXR5RGVzY3JpcHRvcj4=",
                    "sp_entity_id": "https://test_service_provider.com",
                    "kibana_url": "https://test_kibana.com",
                    "attribute_principal": "test_principal",
                    "attribute_groups": "a",
                    "attribute_name": "b",
                    "attribute_email": "c",
                    "attribute_dn": "d"
                }
            }
        ]
    }
    """

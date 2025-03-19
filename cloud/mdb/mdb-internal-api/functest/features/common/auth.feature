Feature: Internal API Auth

    Scenario: Authentication success
        Given default headers
        When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1"
        }
        """
        Then we get gRPC response OK
        When we "GET" via REST at "/mdb/postgresql/1.0/clusters?folderId=folder1"
        Then we get response with status 200

    Scenario: Authentication failure
        Given default headers with "invalid-token" token
        When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
        """
        {
            "folder_id": "folder1"
        }
        """
        Then we get gRPC response error with code UNAUTHENTICATED and message "authentication failed"
        When we "GET" via REST at "/mdb/postgresql/1.0/clusters?folderId=folder1"
        Then we get response with status 403

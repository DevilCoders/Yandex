Feature: Infrastructure works correctly

    Scenario: DBM responds on /ping
        When we issue "GET /ping" request with 60 retries
        Then we get response with code 200
        When we issue "GET /api/version" request with 5 retries
        Then we get response with code 200
         And body is
           """
           version: 2
           """

    Scenario: DBM responds on /metrics
        When we issue "GET /ping" request with 60 retries
        Then we get response with code 200
        When we issue "GET /metrics" request on metrics endpoint
        Then we get response with code 200
         And body contains line starts with
           """
           request_count_total{code="200",method="GET",path="/ping"}
           """

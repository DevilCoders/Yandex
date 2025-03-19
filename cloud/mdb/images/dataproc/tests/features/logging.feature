@skip
Feature: Logging test
    Scenario: Smoke cluster with yarn without cloud logging creates and working
        Given cluster name "smoke"
        And cluster without services
        And cluster topology is singlenode
        And property dataproc:disable_cloud_logging = true
        When cluster created within 5 minutes

    @check
    Scenario Outline: Service on masternodes does not work
        Then service <service> on masternodes is not running
        Examples: masternode services
            | service                        |
            | td-agent-bit                   |

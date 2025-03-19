@conda @trunk
Feature: Cluster with customized python environment works
    Scenario: Cluster with conda creates
        Given cluster name "conda"
        Given NAT network
        And cluster without services
        And cluster topology is singlenode
        And property conda:koalas = >=1.5.0
        And property conda:requests = >2.22.0
        And property conda:psycopg2 = any
        And property conda:ipython = 7.19.0
        And property pip:h3 = 3.7.2
        And property pip:boto3 = <1.18.10
        And property pip:mlflow = >1.*
        And property pip:csvkit = any
        When cluster created within 15 minutes

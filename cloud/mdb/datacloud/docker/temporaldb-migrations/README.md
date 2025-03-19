# Temporal Database Migrations

Docker image which encapsulate dependencies to run Temporal bootstrap operations on the PostgreSQL database.

The image can be used to run the following actions:

- Allow AWS IAM users to authenticate to the AWS RDS PostgreSQL database (set `$AWS_RDS_IAM_AUTH=true`)
- Allow to create `temporal` user on the PostgreSQL database

The image is pushed on the ECR DoubleCloud infraplane prod AWS account.

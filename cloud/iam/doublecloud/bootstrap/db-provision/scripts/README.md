These scripts should be executed once on the services (AS,IAM-CP,RM-CP) started with using of the master key.

These scripts used for:

* create IAM service cloud;
* create folders for IAM-services;
* create ServiceAccounts for IAM-services;
* create authorization keys for IAM-services and store it into [YAV](https://yav.yandex-team.ru/secret/sec-01f4hrse1cnjzkr85v3tns9vqg);
* grant required permissions for IAM-services Service Accounts.

Usage:
1. Forward ports for datacloud-aws-prod https://st.yandex-team.ru/ORION-376#6154b8111a4fee2c5defba76
2. Run scripts on localhost

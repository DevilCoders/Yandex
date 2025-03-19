# dataproc-image tests

## TL;DR

```bash
cd cloud/mdb/images/dataproc/tests
make test
```

## Description

This is a set of infrastructure tests that allows you to test code inside a Data Proc image (bootstrap, cloud-init,
image, salt, modules) without control-plane. The main goal is to be able to test code changes in the image itself,
without use of internal api, metadb, worker. Entire configuration of the Dataproc cluster lives within instance metadata,
so it is possible to provision the cluster without an api and test it.

### Implementation

The tests emulate the control-plane behavior when creating a Data Proc cluster.
* first all required compute resources - vpc networks, subnets, s3 bucket, etc - are created
* special labels are added to all cloud resources: owner (yeah, man, you), branch name and commit hash
* virtual machines are created that, according to metadata, look like a real Data Proc cluster
* at startup, a special flag is passed that disables a launch of the script dataproc-start.sh
* dataproc-agent is also disabled
* local code of all bootstrap entities (dataproc-start.sh, salt modules, states and even dataproc-diagnostics)
  is synchronized to the virtual machines
* dataproc-start.sh is executed
* actual test is executed
* after test execution all created compute resources are destroyed

As a result, you can create a Data Proc cluster with code from a local branch without reassembling images,
which significantly speeds up development and prototyping tasks.

### Resources

By default tests are executed in Compute Preprod:
* folder: [aoe9deomk5fikb8a0f5g](https://console-preprod.cloud.yandex.ru/folders/aoe9deomk5fikb8a0f5g)
* service_account: [bfbh80pcb1s4a9jtg4lf](https://console-preprod.cloud.yandex.ru/folders/aoe9deomk5fikb8a0f5g/service-account/bfbh80pcb1s4a9jtg4lf)
* [service_account credentials](https://yav.yandex-team.ru/secret/sec-01f21mtzjv44s5sv54whxc3gfw/explore/versions)
* [ssh keypair](https://yav.yandex-team.ru/secret/sec-01f2vn3vntm6y7rzhzrryr2xcj)



## Configuration

You can customize your tests config, eg:
```bash
~/g/s/g/s/y/tests ❯❯❯ cat ~/.dataproc-image-test.yaml
save_diagnostics: false
ssh_public_key_file_path: ~/.ssh/id_ecdsa.pub
yav_oauth: ...
s3:
  endpoint: storage.yandexcloud.net
environment:
  folder-id: b1gc248t2adrebpc2unt
  endpoint: api.cloud.yandex.net
vpc: # example of using custom dhcp dns name servers
  v4_cidr_block: '192.168.77.128/27'
  dhcp_options:
    domain_name_servers: ['77.88.8.8']
    domain_name: 'example.com'
```

Default configuration needs only yav_oauth token and uses compute-preprod environment.
YaV OAuth token can be found in https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982

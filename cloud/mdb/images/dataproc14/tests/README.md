# dataproc-image tests
For testing dataproc-image you should have python3 and yandex.cloud folder for runs.
You should create dataproc-images from `packer/` directory before running tests, because tests are require dataproc-image for base start.

## Example:
Create your packer variables config for your folder. This is mine:
`
(venv) ~/g/s/g/s/y/packer ❯❯❯ cat variables-sch-prod.json
{
    "dataproc_agent_version": "1.7142880",
    "env": "prod",
    "folder_id": "b1gc248t2adrebpc2unt",
    "zone": "ru-central1-b",
    "endpoint": "api.cloud.yandex.net:443",
    "pool_size": "1",
    "product_id": "dn2hu09ehnvq16jcp2h0",
    "source_image_id": "fd8fjtn3mj2kfe7h6f0r",
    "enable_gpu": "true"
}`

Actually, this is full copy of compute-prod, except folder_id  and dataproc_agent_version.
After that, you can create new dataproc-images in your folder:
`make dataproc-base ENV=sch-prod`
`make dataproc-image ENV=sch-prod`

Great, now you can customize your tests config, mine is:
`
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
`
Default configuration needs only yav_oauth token and uses compute-preprod environment.
YaV OAuth token can be found in https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982

Okay, now you can return to the `tests/` folder and run `make test`;

## Description
Tests emulates dataproc service, but doesn't use control-plane. They are manually disables bootstrap highstate and dataproc-agent, and they are good for testing salt and image changes.
They are automatically creates all required environment to your folder (vpc networks, subnets, s3 bucket, etc) and delete them after all.
After that, tests a creates instances with special metadata and hostnames, and emulates real yandex cloud dataproc cluster.
Tests syncs all bootstrap entities (salt modules, states, bootstrap script and even dataproc-diagnostics) and then manually runs bootstrap.

Also, they are add special labels on all cloud resources: owner (yeah, man, you), branch name and commit hash.

Good luck! Remember, green tests better than red ones!

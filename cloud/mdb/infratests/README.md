# Infra-tests and dev-stands
## Configuration
### Set up kubectl
[Follow this guide](https://wiki.yandex-team.ru/mdb/internal/k8s/gettings-started-with-k8s/#configurekubectlandycprofile)
### Configure Docker
In order to build and push Docker images locally, you need to configure Docker.
#### MacOS installation
[Guide blog post](https://maxlog.at.yandex-team.ru/149)

If you are running on ARM machine, you have to build Docker for x86_x64. (Re)start colima in a specific way:
```shell
$ colima status
INFO[0000] colima is running
INFO[0000] runtime: docker
INFO[0000] arch: aarch64
$ colima delete # if you already have running colima for arm
$ colima start --arch x86_64
$ colima status
INFO[0000] colima is running
INFO[0000] runtime: docker
INFO[0000] arch: x86_64
```
### Configure registries credentials
#### Yandex Registry
[Follow this guide](https://wiki.yandex-team.ru/qloud/docker-registry/#authorization)
#### Cloud Registries
Easiest way to authorize with cloud registries is to configure a Docker Cred Helper.
Start from running:
```
$ yc container registry configure-docker
Credential helper is configured in '/home/<user>/.docker/config.json
```
Then you might need to add preprod registry to `/.docker/config.json`. It should contain following:
```json
{
	"auths": {
		"registry.yandex.net": {
			"auth": "xxx"
		}
	},
	"credHelpers": {
		"container-registry.cloud.yandex.net": "yc",
		"cr.cloud-preprod.yandex.net": "yc",
		"cr.cloud.yandex.net": "yc",
		"cr.yandex": "yc"
	},
	"currentContext": "colima"
}
```
To map `yc` profile with specific registry edit or create `/Users/<user>/.config/yandex-cloud/credhelper-config.yaml`.
It should look similar to:
```yaml
container-registry.cloud-preprod.yandex.net: yc-compute-preprod-infratest
cr.cloud-preprod.yandex.net: yc-compute-preprod-infratest
default: yc-compute-prod-cp
```

## Run stand
Clone repo with helm or use existing: https://review.db.yandex-team.ru/admin/repos/mdb/controlplane

Within arcadia/cloud/mdb/infratests:
```bash
make gen_env
make fill_values
```

Create stand with helmfile:
```bash
export NAMESPACE=$STAND
cd $HELMFILE_DIR_PATH
helmfile apply
```

Come back to arcadia/cloud/mdb/infratests and finish stand initialization:
```bash
cd -
ACTION=create_dns_records ./infratests provision
ACTION=prepare ./infratests provision
```

Currently, valid resources are not populated with helm, so we have to do it manually:
```bash
kubectl exec -it mdb-metadb-0 --namespace $NAMESPACE -- bash
psql --user postgres
```
```postgresql
\c dbaas_metadb
insert into dbaas.valid_resources values (9554, 'hadoop_cluster', 'hadoop_cluster.masternode', 'b7e4600e-8356-4c98-88fb-bfce55e4467b', 3, 3, '[10737418240,4398046511105)', null, 1, 1, null);
insert into dbaas.valid_resources values (8372, 'hadoop_cluster', 'hadoop_cluster.datanode', 'b7e4600e-8356-4c98-88fb-bfce55e4467b', 3, 3, '[10737418240,4398046511105)', null, 1, 32, null);
insert into dbaas.valid_resources values (8963, 'hadoop_cluster', 'hadoop_cluster.computenode', 'b7e4600e-8356-4c98-88fb-bfce55e4467b', 3, 3, '[10737418240,4398046511105)', null, 0, 32, null);
insert into dbaas.valid_resources values (9557, 'hadoop_cluster', 'hadoop_cluster.masternode', 'a5fdad63-163d-473f-a0a2-12f889942217', 3, 3, '[10737418240,4398046511105)', null, 1, 1, null);
insert into dbaas.valid_resources values (8375, 'hadoop_cluster', 'hadoop_cluster.datanode', 'a5fdad63-163d-473f-a0a2-12f889942217', 3, 3, '[10737418240,4398046511105)', null, 1, 32, null);
insert into dbaas.valid_resources values (8966, 'hadoop_cluster', 'hadoop_cluster.computenode', 'a5fdad63-163d-473f-a0a2-12f889942217', 3, 3, '[10737418240,4398046511105)', null, 0, 32, null);

insert into dbaas.valid_resources values (9566, 'hadoop_cluster', 'hadoop_cluster.masternode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 3, '[10737418240,4398046511105)', null, 1, 1, null);
insert into dbaas.valid_resources values (8384, 'hadoop_cluster', 'hadoop_cluster.datanode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 3, '[10737418240,4398046511105)', null, 1, 32, null);
insert into dbaas.valid_resources values (8975, 'hadoop_cluster', 'hadoop_cluster.computenode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 3, '[10737418240,4398046511105)', null, 0, 32, null);

insert into dbaas.valid_resources values (9568, 'hadoop_cluster', 'hadoop_cluster.masternode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 2, '[10737418240,4398046511105)', null, 1, 1, null);
insert into dbaas.valid_resources values (8386, 'hadoop_cluster', 'hadoop_cluster.datanode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 2, '[10737418240,4398046511105)', null, 1, 32, null);
insert into dbaas.valid_resources values (8977, 'hadoop_cluster', 'hadoop_cluster.computenode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 2, '[10737418240,4398046511105)', null, 0, 32, null);

insert into dbaas.valid_resources values (9567, 'hadoop_cluster', 'hadoop_cluster.masternode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 1, '[10737418240,4398046511105)', null, 1, 1, null);
insert into dbaas.valid_resources values (8385, 'hadoop_cluster', 'hadoop_cluster.datanode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 1, '[10737418240,4398046511105)', null, 1, 32, null);
insert into dbaas.valid_resources values (8976, 'hadoop_cluster', 'hadoop_cluster.computenode', 'cd0edc1c-628b-42dd-b954-8f28acdc22c4', 3, 1, '[10737418240,4398046511105)', null, 0, 32, null);
```
Exit with ^D, ^D.

Run tests with
```bash
./infratests features/dataproc/dataproc_cli.feature
```

To delete stand run
```bash
make clean
```

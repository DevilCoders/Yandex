# Yandex.Cloud Quota Manager for Support team

The Quota Manager allows you to view and manage quota metrics for supported YC services.

REST and gRPC API supported.

## Get started

### Build binary

Use `ya make` to build binary.
[Guide: ya make](https://wiki.yandex-team.ru/yatool/make/)

### Config setup

**Get OAuth token** [here](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb)

Default config path: `~/.config/qctl.yaml`

You can also use your own path to the config file:
`qctl --config /path/to/config.yaml`

**Config example:**
```yaml
token: AQAAqweqwezxczxcqweqweXczczsw
ssl: /path/to/AllCAs.pem
billing_metadata: false

# optional (if not specified, standard endpoints will be used)
endpoints:
  prod:
    compute: https://example.compute.ru/api/v1
    managed-database: https://example.mdb.ru/api/v1:443
    object-storage: ...
    resource-manager: ...
    kubernetes: ...
    triggers: ...
    functions: ...
    instance-group: ...
    container-registry: ...

  preprod:
    compute: ...
    ...
```

If `billing_metadata` is set to true — the billing account balance, status, and other data 
will be displayed in interactive mode.

You can also pass the token and certificate as arguments.  
The token argument can accept both a IAM and OAuth token.  

Example:
```bash
qctl --token {IAM or OAuth token} --ssl ~/allCAs.pem
```

### Usage

#### Examples

* Show preprod mdb quota:
```bash
qctl -mdb b1qqqwwwweeerrrtttyyy --preprod
```

* Interactive set compute quota:
```
qctl --compute b1qqqwwwweeerrrtttyyy --set
```

* Set concrete functions quota:
```bash
qctl --functions b1qqqwwwweeerrrtttyyy -s -n serverless.memory.size -l 25G
```

* Set quota metrics to zero:
```bash
qctl --zeroize b1qqqwwwweeerrrtttyyy --services=compute,mdb,s3
```

* Set quota metrics to default for all services:
```bash
qctl --default b1qqqwwwweeerrrtttyyy
```

* Set quota from yaml:

YAML example:
```yaml
cloud_id: b1qqqwwwweeerrrtttyyy
service: compute
metrics:
  - name: compute.instanceCores.count
    limit: 200
  - name: compute.snapshots.count
    limit: 200
  - name: compute.ssdDisks.size
    limit: 5T
```

**The argument --subject overwrites the cloud_id from the file.**

Example for concrete cloud_id:
```bash
qctl --set-from-yaml file.yaml
```

Example for many clouds:
```bash
for cloud in `cat clouds.txt`; do qctl --set-from-yaml file.yaml --subject cloud; done
```

#### Help message

```
usage: qctl [service] cloud_id [subcommands] [options]

  qctl                                                                     Run interactive mode
  qctl --compute <cloud_id>                                                Show compute quota metrics
  qctl --compute <cloud_id> --set                                          Interactive set compute quota metrics
  qctl --compute <cloud_id> --set --name <metric> --limit <value>          Set limit for concrete quota metric

Yandex.Cloud Quota Manager

optional arguments:
  -h, --help            show this help message and exit
  --version             show program's version number and exit


Commands:
  --compute cloud, -c cloud
  --container-registry cloud, -cr cloud
  --instance-group cloud, -ig cloud
  --kubernetes cloud, -k8s cloud
  --managed-database cloud, -mdb cloud
  --object-storage cloud, -s3 cloud
  --resource-manager folder, -rm folder
  --functions cloud, -f cloud
  --triggers cloud, -t cloud
  --virtual-private-cloud cloud, -vpc cloud
  --load-balancer cloud, -lb cloud
  --monitoring cloud, -m cloud
  --internet-of-things cloud, -iot cloud


Subcommands of [service]:
  --set, -s             set quota metrics
  --name str, -n str    quota metric name [for --set]
  --limit str, -l str   quota metric limit [for --set]
  --multiply N          multiply service metrics to multiplier
  --show-balance        show billing metadata in interactive mode


Other commands:
  --zeroize cloud       set metrics to zero
  --default cloud       set metrics to default
  --services services   service1,service2,.. [for --zeroize/default]

  --code code, -q code  update metrics from quotacalc

  --set-from-yaml file  set quota from yaml
  --subject cloud       optional id for --set-from-yaml


Options:
  --config file         path to config file
  --token str           iam or oauth token
  --ssl file            path to AllCAs.pem
  --preprod             preprod environment
  --debug, -v           debug mode
```


## How to add new service

Simple guide for those who want to participate in the development.

```
* Create new module in quota/services/{service_name}.py
* Copy class from any existent service module
* Edit new class
* Import created class in to quota/services/__init__.py
* Add argument to cli/__init__.commands
* Add service metadata to:  
  - constants.SERVICES
  - constants.SERVICE_ALIASES  # if service name like a "long-service-name"
  - quota/utils/request/gRPCRequest.__SERVICES__  # if service uses a gRPC client (don't forget to import pb2 module)
* quotacalc: request.py import grpc module for service
* quotacalc: workers.py add service to SERVICES_ACTIVE
* Checkout and enjoy!
```

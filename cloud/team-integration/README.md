# Integration with Yandex internal services

Full list: https://wiki.yandex-team.ru/intranet/

## Build instructions

```bash
ya make -tt
```

To build installation package

```bash
ya package --debian package/yc-team-integration-package.json
```

To update cloud-java dependencies:

```
./update-java-common.sh <cloud-java-release> <proto-java-release>
```

for example, to update dependencies to cloud-java release `20603-c7ed3c926c`
and proto-java release `1.0.11804-5140d72298`:

```
$ ./update-java-common.sh 20603-c7ed3c926c 1.0.11804-5140d72298
$ ya make -tt
$ ya pr c -m "Update cloud-java dependencies"
```

* The latest release of cloud-java common is [here](https://teamcity.yandex-team.ru/buildConfiguration/cloud_java_CloudJavaRepositoryRelease?branch=%3Cdefault%3E).
* The latest release of private API is [here](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_CloudGo_PrivateApiBuild?branch=%3Cdefault%3E).

These commands should be executed in the folder `arcadia/cloud/team-integration`
(the root of the project).

## Local run

To debug the service run `DebugApplication` class. It works the same way as the `Application`
class, but requires no TVM authentication and uses in-memory DB. It also implements fake
`ResourceManager` and `ABC` services.
The debug configuration is stored in `src/test/resources/debug-application.yaml`.

To debug the service in **production** configuration the following services are required:

* YDB, available as docker image here: https://ydb.yandex-team.ru/docs/getting_started/ydb_docker
* Access service
* Token service
* IAM control plane service
* RM control plane service

## Bootstrap (for local debug)

* Create service account:

```bash
grpcurl -plaintext -H 'Authorization: Bearer ...' \
  -d '{"id": "yc.iam.sync", "folder_id": "garden", "name": "yc-iam-sync"}' \
  localhost:4283 yandex.cloud.priv.iam.v1.ServiceAccountService/Create
```

* Create key for new service account:

```bash
grpcurl -plaintext -H 'Authorization: Bearer ...' \
  -d '{"key_id": "yc.iam.sync", "service_account_id": "yc.iam.sync"}' \
  localhost:4283 yandex.cloud.priv.iam.v1.KeyService/Create | jq -rc .private_key
```

## ABC Service

GRPC service to resolve cloud_id for Yandex.Cloud from ABC slug or ABS id and vise versa.
See https://wiki.yandex-team.ru/intranet/abc/

### API

```proto
service AbcService {
  // Creates new cloud for abc slug or id.
  // If no abc service exists for the given slug/id, the result will be `FAILED_PRECONDITION`.
  // If such cloud already exists for the given slug/id, the result will be `ALREADY_EXISTS`.
  rpc Create (CreateCloudRequest) returns (operation.Operation) {
    option (yandex.cloud.api.operation) = {
        metadata: "CreateCloudMetadata"
        response: "CreateCloudResponse"
      };
  }

  // Resolves (cloud id, abc slug, abc id) for any of them.
  // If no cloud exists for the given abc service, it will be created automatically.
  // If no abs service exists for the given cloud, the result will be `NOT FOUND`.
  rpc Resolve (ResolveRequest) returns (ResolveResponse) {}
}

message ResolveRequest {
  oneof abc {
    option (exactly_one) = true;
    string cloud_id = 1;
    string abc_slug = 2;
    int64 abc_id = 3;
    string abc_folder_id = 4;
  }
}

message ResolveResponse {
  string cloud_id = 1;
  string abc_slug = 2;
  int64 abc_id = 3;
  string default_folder_id = 4;
  string abc_folder_id = 5;
}

message CreateCloudRequest {
  oneof abc {
    option (exactly_one) = true;
    string abc_slug = 2;
    int64 abc_id = 3;
    string abc_folder_id = 4;
  }
}

message CreateCloudMetadata {
  string abc_slug = 1;
  int64 abc_id = 2;
  string abc_folder_id = 3;
}

message CreateCloudResponse {
  string cloud_id = 1;
  string default_folder_id = 2;
  string abc_folder_id = 3;
}
```

For testing:

```bash
grpcurl -v -H "X-Request-Id: $(uuidgen -t)" -H "Authorization: Bearer $IAM_TOKEN" -d '{"abc_id": 2}' \
  ti.cloud.yandex-team.ru:443 yandex.cloud.priv.team.integration.v1.AbcService/Resolve

grpcurl -v -H "X-Request-Id: $(uuidgen -t)" -H "Authorization: Bearer $IAM_TOKEN" -d '{"abc_slug": "home"}' \
  ti.cloud.yandex-team.ru:443 yandex.cloud.priv.team.integration.v1.AbcService/Resolve

grpcurl -v -H "X-Request-Id: $(uuidgen -t)" -H "Authorization: Bearer $IAM_TOKEN" -d '{"abc_id": 2}' \
  ti.cloud.yandex-team.ru:443 yandex.cloud.priv.team.integration.v1.AbcService/Create

grpcurl -v -H "X-Request-Id: $(uuidgen -t)" -H "Authorization: Bearer $IAM_TOKEN" -d '{"abc_slug": "home"}' \
  ti.cloud.yandex-team.ru:443 yandex.cloud.priv.team.integration.v1.AbcService/Create

grpcurl -v -H "X-Request-Id: $(uuidgen -t)" -H "Authorization: Bearer $IAM_TOKEN" -d '{"operation_id": "bg312345678901234567"}' \
  ti.cloud.yandex-team.ru:443 yandex.cloud.priv.team.integration.v1.OperationService/Get
```

## IDM Service

REST WEB-server to serve IDM requests.
See https://wiki.yandex-team.ru/intranet/idm/

### API

* /idm/info

* /idm/add-role

* /idm/remove-role

* /idm/get-roles

For testing:

```bash
curl -H "X-Ya-Service-Ticket: $TVM_TICKET" -v https://idm.ti.cloud.yandex-team.ru/idm/info

curl -H "X-Ya-Service-Ticket: $TVM_TICKET" -H "Content-Type: application/x-www-form-urlencoded" -v https://idm.ti.cloud.yandex-team.ru/idm/add-role \
  -d 'path=%2Fcloud%2Fs3%2Fstorage%2Fadmin%2F&role=%7B%22cloud%22%3A+%22s3%22%2C+%22storage%22%3A+%22admin%22%7D&login=user1-id&fields=%7B%22service_spec%22%3A+%22home-2-42868%22%7D'

curl -H "X-Ya-Service-Ticket: $TVM_TICKET" -H "Content-Type: application/x-www-form-urlencoded" -v https://idm.ti.cloud.yandex-team.ru/idm/remove-role \
  -d 'path=%2Fcloud%2Fs3%2Fstorage%2Fadmin%2F&role=%7B%22cloud%22%3A+%22s3%22%2C+%22storage%22%3A+%22admin%22%7D&login=user1-id&fields=%7B%22service_spec%22%3A+%22home-2-42868%22%7D'

curl -H "X-Ya-Service-Ticket: $TVM_TICKET" -v https://idm.ti.cloud.yandex-team.ru/idm/get-roles
```

## ABCD Adapter

https://wiki.yandex-team.ru/resource-model/providers/provider_iface_spec/

For testing:

```bash
grpcurl -H "X-Ya-Service-Ticket: $TVM_TICKET" \
  -d '{"abc_service_id": 2, "folder_id": "123", "provider_id": "123", "key": "some_key", "display_name": "name"}' \
  ti.cloud.yandex-team.ru:443 \
  intranet.d.backend.service.provider_proto.AccountsService/CreateAccount

grpcurl -H "X-Ya-Service-Ticket: $TVM_TICKET" \
  -d '{"abc_service_id": 2, "folder_id": "123", "provider_id": "123", "account_id": "123"}' \
  ti.cloud.yandex-team.ru:443 \
  intranet.d.backend.service.provider_proto.AccountsService/DeleteAccount

grpcurl -H "X-Ya-Service-Ticket: $TVM_TICKET" \
  -d '{"account_id": "123", "folder_id": "123", "abc_service_id": 123, "provider_id": "123"}' \
  ti.dev.cloud.yandex-team.ru:443 \
  intranet.d.backend.service.provider_proto.AccountsService/GetAccount

grpcurl -H "X-Ya-Service-Ticket: $TVM_TICKET" \
  ti.cloud.yandex-team.ru:443 \
  intranet.d.backend.service.provider_proto.AccountsService/ListAccounts

grpcurl -H "X-Ya-Service-Ticket: $TVM_TICKET" \
  -d '{"folder_id":"123"}' \
  ti.cloud.yandex-team.ru:443 \
  intranet.d.backend.service.provider_proto.AccountsService/ListAccountsByFolder

```

## Health check (HTTP)

```bash
curl -v https://idm.ti.cloud.yandex-team.ru/ping
```

# Access Service C++ client library

## Prerequisites

* GRPC 1.30+
* CMAKE 3.12+
* C++2A compatible compiler

## Build

```bash
$ git submodule update

$ mkdir build

$ cd build

$ cmake ..

$ cmake --build .
```

## Usage

```c++
#include <cloud-auth/client.h>

AccessServiceClientConfig cfg {
    .ClientName = "as-client-test/1.0",
    .Endpoint = "localhost:4286",
    .KeepAliveTime = 60s,
    .KeepAliveTimeout = 10s,
    .Plaintext = false,
    .RetryPolicy = {
        .MaxAttempts = 7,
        .InitialBackOff = 0.1s,
        .MaxBackOff = 2s,
        .BackoffMultiplier = 1.6,
        .RetryableStatusCodes = {
            "ABORTED",
            "CANCELLED",
            "DEADLINE_EXCEEDED",
            "INTERNAL",
            "UNAVAILABLE",
            "UNKNOWN",
        }
    },
    .RootCertificate = ReadFile("/etc/ssl/certs/YandexInternalRootCA.pem"),
    .SslTargetNameOverride = "as.private-api.cloud.yandex.net",
};

auto asyncClient = AccessServiceAsyncClient::Create(cfg);

asyncClient->Authorize(
        IamToken("t1.aaaa....aaaa"),
        "service.resource.permission",
        {Resource::Folder("folder_id")},
        [](const auto& result) {
           if (result.GetStatus() == AuthStatus::ERROR) {
               // handle GRPC error
           }
           std::cout << result << std::endl;
       });
```

## API

```c++
using namespace yandex::cloud::auth;

auto authClient = AccessServiceClient::Create(cfg);
auto asyncClient = AccessServiceAsyncClient::Create(cfg);
```

### AuthStatus

```c++
enum AuthStatus {
    ERROR, // IO error
    OK, // Succeeded
    UNAUTHENTICATED, // Invalid credentials
    PERMISSION_DENIED, // No access
};
```

### Credentials

```c++
class IamToken {
public:
    const std::string& Value();
};

class ApiKey {
public:
    const std::string& Value();
};

enum SignatureMethod {
    SIGNATURE_METHOD_UNSPECIFIED,
    HMAC_SHA1,
    HMAC_SHA256,
};

class Version2Parameters {
public:
    SignatureMethod GetSignatureMethod();
};

class Version4Parameters {
public:
    const std::chrono::time_point<std::chrono::system_clock>& SignedAt();
    const std::string& Region();
    const std::string& Service();
};

class AccessKeySignature {
public:
    using AccessKeySignatureParameters = std::variant<Version2Parameters, Version4Parameters>;

    const std::string& AccessKeyId();
    const std::string& SignedString();
    const std::string& Signature();
    const AccessKeySignatureParameters& Parameters();
};
using Credentials = std::variant<IamToken, ApiKey, AccessKeySignature>;
```

### Error

```c++
class Error {
public:
    int32_t Code(); // Implementation specific error code
    const std::string& Message(); // User-friendly message
    const std::string& Details(); // Internal details
};
```

### Authenticate

```c++
AuthenticationResult Authenticate(const Credentials& credentials);

class AuthenticationResult {
public:
    explicit operator bool() const {
        return Status_ == AuthStatus::OK;
    }

    AuthStatus Status();
    const Subject& GetSubject();
    const Error& GetError();
```

Example:

```c++
auto result = authClient->Authenticate(IamToken("t1.abcd....xyz"));
if (result) {
    log("Authenticated as: " + result.GetSubject())
} else {
    log("Authentication failed " + result.GetError())
}
```

### Authenticate (async)

```c++
std::future<AuthenticationResult> Authenticate(const Credentials& credentials);

void Authenticate(
        const Credentials& credentials,
        std::function<void(const AuthenticationResult&)> callback);
```

Example:

```c++
// With std::future
auto future = asyncClient->Authenticate(IamToken("t1.abcd....xyz"));
// ...
auto result = future.get();
if (result) {
    log("Authenticated as: " + result.GetSubject())
} else {
    log("Authentication failed " + result.GetError())
}

// With callback
asyncClient->Authenticate(IamToken("t1.abcd....xyz"),
    [](const auti& result) -> {
        if (result) {
            log("Authenticated as: " + result.GetSubject())
        } else {
            log("Authentication failed " + result.GetError())
    });
}
```

### Authorize

```c++
AuthorizationResult Authorize(
        const Credentials& credentials,
        const std::string& permission,
        const Resource& resource);

class AuthorizationResult {
public:
    explicit operator bool() const {
        return Status_ == AuthStatus::OK;
    }

    AuthStatus Status();
    const Subject& GetSubject();
    const Error& GetError();
```

Example:

```c++
auto result = authClient->Authorize(
        IamToken("t1.abcd....xyz"),
        "resource-manager.clouds.get",
        {Resource("foo1234345", "hogwarts.wand"), Resource::Folder("bar123")});

switch (result.GetStatus()) {
    case AuthStatus::ERROR:
        log("Request failed " + result.GetError());
        break;
    case OK:
        log("Authenticated as: " + result.GetSubject());
        break;
    case UNAUTHENTICATED:
        log("Authentication failed: " + result.GetError());
        break;
    case PERMISSION_DENIED:
        log("Permission denied for: " + result.GetSubject());
        break;
}
```

### Authorize (async)

```c++
std::future<AuthorizationResult> Authorize(
        const Credentials& credentials
        const std::string& permission,
        const Resource& resource);

void Authorize(
        const Credentials& credentials,
        const std::string& permission,
        const Resource& resource,
        std::function<void(const AuthorizationResult&)> callback);
```

Example:

```c++
// With std::future
auto future = asyncClient->Authorize(
    ApiKey("AAA...ZZZ"),
    "resource-manager.clouds.get",
    {Resource::Cloud("bar123")});
// ...
auto result = future.get();
if (result) {
    log("Authenticated as: " + result.GetSubject())
} else {
    log("Authentication failed " + result.GetError())
}

// With callback
asyncClient->Authorize(
    AccessKeySignature(
        "SomeAccessKeyId",
        "SomeString",
        "abcd123456..ef",
        Version2Parameters(yandex::cloud::auth::HMAC_SHA256)
    ),
    "test.permission",
    {Resource::Cloud("bar123")}),
    [](const auti& result) -> {
        // ...
    }
}
```

## Troubleshooting

### Local IAM setup

Required services:
 
* Access Service
* Token Service
* IAM Control Plane

Services should be running in the "seed" mode.

### YCP config

```yaml
environments:
  local:
    service-control:
      endpoint:
        address: localhost:4286
        plaintext: true
    iam:
      v1:
        mfa:
          endpoint:
            address: localhost:6327
        services:
          iam-token:
            address: localhost:4282
            plaintext: true
      endpoint:
        address: localhost:4283
        plaintext: true
    resource-manager:
      endpoint:
        address: localhost:4284
        plaintext: true
```

### To issue IAM Token for service account "eve"

```bash
$ YC_IAM_TOKEN=MASTER-TOKEN ycp --profile local iam iam-token create-for-subject -r - <<< "subject_id: eve"
Cgg...MAI=
```

### To create access key for account "foo70000000000000000"

```bash
$ YC_IAM_TOKEN=MASTER-TOKEN ycp --profile local iam user-account \
  presign-url -r - <<< "{string_to_sign: test, subject_id: foo70000000000000000, v2_parameters: {signature_method: HMAC_SHA256} }"
access_key_id: Wu6...5UV
signature: yf9...MQ=

$ YC_IAM_TOKEN=MASTER-TOKEN ycp --profile local iam user-account \
  presign-url -r - <<< "{string_to_sign: test, subject_id: foo70000000000000000, v4_parameters: {region: central-1a, service: test-service, signed_at: 2021-11-22T16:46:00Z}}"
access_key_id: Wu6...5UV
signature: 400...5A9
```

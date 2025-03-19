# Gauthling Mock Server

This server simulates Gauthling GRPC interface. Authentication/authorization results can be controlled at runtime 
via HTTP control server.

By default mock server is on `localhost:4284`, control server is on `localhost:2484`.

## Control server request examples

Display current state:
```bash
$ curl http://localhost:2484/gauthling/state
{
  "authenticate_requests": true,
  "authorize_requests": true
}
```

Set gauthling not to authenticate incoming requests:
```bash
$ curl -X POST -H 'Content-Type: application/json' -d '{"value": false}' http://localhost:2484/gauthling/setAuthenticateRequests
{
  "status": "OK"
}
```

Set gauthling to authorize incoming requests:
```bash
$ curl -X POST -H 'Content-Type: application/json' -d '{"value": true}' http://localhost:2484/gauthling/setAuthorizeRequests
{
  "status": "OK"
}
```

Set new gauthling API to authenticate any incoming request:
```bash
$ curl -X POST -H 'Content-Type: application/json' -d '{"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}}' http://localhost:2484/access_service/setAuthenticateRequestsSubject
{
  "status": "OK"
}
```

Set new gauthling API to reject any authentication incoming request:
```bash
$ curl -X POST -H 'Content-Type: application/json' -d 'null' http://localhost:2484/access_service/setAuthenticateRequestsSubject
{
  "status": "OK"
}
```

Set new gauthling API to authorize any incoming request:
```bash
$ curl -X POST -H 'Content-Type: application/json' -d '{"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}' http://localhost:2484/access_service/setAuthorizeRequestsSubject
{
  "status": "OK"
}
```

Set new gauthling API to reject any authorization incoming request:
```bash
$ curl -X POST -H 'Content-Type: application/json' -d 'null' http://localhost:2484/access_service/setAuthorizeRequestsSubject
{
  "status": "OK"
}
```

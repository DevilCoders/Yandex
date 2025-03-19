# Access Service Mock Server

## Standalone mode

Start the mock server

```bash
$ ya make && ./python
```

Check if the server is alive

```bash
$ curl http://localhost:2484/ping
```

Mock the authentication subject

```bash
$ curl -H 'Content-Type: application/json' -X PUT --data '{"userAccount": {"id": "..."}}' http://localhost:2484/authenticate
```

Setting the mocked value to null will result in Unauthneticated responses

```bash
$ curl -H 'Content-Type: application/json' -X PUT --data 'null' http://localhost:2484/authenticate
```

Get the mocked authentication subject

```bash
$ curl http://localhost:2484/authenticate
```

Mock the authorization subject

```bash
$ curl -H 'Content-Type: application/json' -X PUT --data '{"serviceAccount": {"id": "...", "folderId": "..."}}' http://localhost:2484/authorize
```

Setting the mocked value to null will result in PermissionDenied responses

```bash
$ curl -H 'Content-Type: application/json' -X PUT --data 'null' http://localhost:2484/authorize
```

Get the mocked authorization subject

```bash
$ curl http://localhost:2484/authorize
```

Stop the server

```bash
$ curl -X POST http://localhost:2484/shutdown
```

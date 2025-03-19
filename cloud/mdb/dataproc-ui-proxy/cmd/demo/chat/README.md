This program's purpose is to test proxying of websocket connections through dataproc-ui-proxy.
Websocket proxy feature includes javascript code to be run by a browser which is troublesome
to test with unit tests. This demo application simplifies manual testing of the feature.

Run with:
```shell script
ya make
./chat
```

This will run a proxy server, proxy agent and backend chat server.
Open web browser at http://localhost:8080, open inspector and enjoy.

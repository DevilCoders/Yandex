#!/bin/sh

terraform init $* -backend-config="secret_key=$(ya vault get version sec-01eqk8sxxncwfdk35d0zt86tpn -o secret_key)"

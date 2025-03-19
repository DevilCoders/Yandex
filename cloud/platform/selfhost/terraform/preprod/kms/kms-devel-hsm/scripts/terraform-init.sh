#!/bin/sh

terraform init $* -backend-config="secret_key=$(ya vault get version sec-01emgjtvj72zvjkv333wkv1cnb -o secret_key)"

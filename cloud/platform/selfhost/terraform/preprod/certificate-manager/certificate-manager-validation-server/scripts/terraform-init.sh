#!/bin/sh
terraform init $* -backend-config="secret_key=$(ya vault get version sec-01dvncym5jp9y9115d6bh5f36e -o secret_key)"

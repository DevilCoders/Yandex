#!/bin/sh

terraform init $* -backend-config="secret_key=$(ya vault get version sec-01djn2s8h6b657he0hy6hfhpvf -o access-secret-key-terraform-maintainer)"

#!/bin/sh
terraform init $* -backend-config="secret_key=$(ya vault get version sec-01dzxtbv7qfj93t00wjznvfcqs -o secret_key)"

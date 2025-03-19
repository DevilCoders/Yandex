#!/usr/bin/env bash
terraform init -backend-config="secret_key=${KOZHAPENKO_DEVEL_SA_TF_KEY}"

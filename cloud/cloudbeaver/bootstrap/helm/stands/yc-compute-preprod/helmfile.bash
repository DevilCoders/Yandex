#!/bin/bash

KUBECONFIG=./.kubeconfig helmfile "$@"

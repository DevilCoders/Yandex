#!/usr/bin/env bash

set -e

yc-compute-admin db online-populate

yc-compute-populate-devel-database

#!/bin/bash

set -e

project_dir="`cd $(dirname $0)/.. && pwd`"
project_name="mstand-binary"

. "${project_dir}/../../quality/yaqlib/yaqutils/build_package_common_functions.sh"

latest_package_path="package/${project_name}.latest.tar.gz"

prev_num="$1"
build_project_package "${project_dir}" "${project_name}" "${prev_num}" --dist -E --checkout

#!/bin/bash

set -e

project_dir="`cd $(dirname $0)/.. && pwd`"
rm -rf `find "$project_dir/user_plugins/user_module" -type d`

project_name="mstand"

. "${project_dir}/../../quality/yaqlib/yaqutils/build_package_common_functions.sh"


latest_package_path="package/${project_name}.latest.tar.gz"

if [ "$1" = "-l" ]; then
    build_extended=true
    shift
else
    build_extended=false
fi

prev_num="$1"
build_project_package "${project_dir}" "${project_name}" "${prev_num}"

package_path=`cat "${latest_package_path}.package"`
package_version=`cat "${latest_package_path}.version"`

if $build_extended; then
    echo "building extended mstand package"
    # MSTAND_EXTENDED - libra
    ya package --checkout mstand-libra-files.json
fi

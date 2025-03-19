#!/usr/bin/env bash

set -e
[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x

platforms="linux darwin"
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--platform)
            case $2 in
                linux|darwin)
                    platforms=$2
                    ;;
                all)
                    ;;
                *)
                    echo "\"$2\" is an invalid value for --platform. Possible values: \"linux\", \"darwin\" and \"all\"."
                    exit 1
                    ;;
            esac
            shift 2
            ;;
        -r|--revision)
            revision="$2"
            shift 2
            ;;
        -h|--help)
            help=1
            shift
            break
            ;;
        --)
            shift
            break
            ;;
        -*)
            help=1
            error=1
            break
            ;;
        *)
            break
            ;;
    esac
done

if [[ $# -ne 2 ]]; then
    help=1
    error=1
fi

if [[ "$help" == 1 ]]; then
    cat <<EOF
Usage: `basename $0` [<option>] ... <s3_access_key> <s3_secret_key>
  -p, --platform <value>  Target platform. Possible values: "linux", "darwin" and "all". Default is "all".
  -r, --revision          Source code revision. By default, the value is determined automatically using "svn info".
  -h, --help              Show this help message and exit.
EOF
    exit ${error:0}
fi

s3_access_key="$1"
s3_secret_key="$2"


search_path=$(pwd)
while [ "$search_path" != "/" ]; do
    search_path=$(dirname "$search_path")
    if [[ $(find "$search_path" -maxdepth 1 -name ya) ]]; then
        arcadia_root=${search_path}
        break
    fi;
done


if [[ -z "$revision" ]]; then
    revision="$(cd ${arcadia_root}/cloud/mdb && svn info --show-item last-changed-revision)"
fi
version=$(sed -e "s/dev0/${revision}/" dbaas/version.txt)


echo "Building cli tools $version"
mkdir build
echo "$version" > build/current_version
for platform in $platforms; do
    echo "$platform build"
    (set -x; ${arcadia_root}/ya make --checkout --target-platform $platform -r ${arcadia_root}/cloud/mdb/cli ${arcadia_root}/cloud/mdb/mdb-cli)
    mkdir -p "build/${version}/${platform}/"
    cp ${arcadia_root}/cloud/mdb/cli/dbaas/dbaas "build/${version}/${platform}/"
    cp ${arcadia_root}/cloud/mdb/mdb-cli/cmd/mdb-admin/mdb-admin "build/${version}/${platform}/"
done


echo "Uploading build artifacts to https://console.cloud.yandex.ru/folders/b1gkpg571s7am7h917sr/storage/bucket/mdb-scripts"
s3cmd="s3cmd --host storage.yandexcloud.net --host-bucket %(bucket)s.storage.yandexcloud.net --region ru-central1 --access_key $s3_access_key --secret_key $s3_secret_key"
$s3cmd put --recursive build/* s3://mdb-scripts/

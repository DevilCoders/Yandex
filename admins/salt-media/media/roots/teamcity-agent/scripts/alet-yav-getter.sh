#!/usr/bin/env bash

set -e

function help_print() {

    echo "Description:"
    echo "This script gets last version of secret from YAV."
    echo "usage : <secret_id> <secret_type> optional<file_name> / help"
    echo "        <help>: print this message"
    echo "        <secret_id>: secret id name like this sec-01czr64jsrf7qxgxrdcgm7y3v8"
    echo "        <secret_type>: file or var"
    echo "        <secret_file_name>: file name in YAV"
    exit 0
}

function yav_secret_file_get() {
    local secret_id="$1"
    local verison_id=""
    local file_id="$2"
    # get latest version
    verison_id="$(yav get secret "$secret_id" -j | jq -r '.["secret_versions"] | min_by("created_at") | .["version"]')"
    # get latest secret application.properties
    yav get version "$verison_id" -o "$file_id"

}

function yav_secret_var_get() {
    local secret_id="$1"
    local verison_id=""
    # get latest version
    verison_id="$(yav get secret "$secret_id" -j | jq -r '.["secret_versions"] | min_by("created_at") | .["version"]')"
    yav get version "$verison_id" -j | jq -r  '.["value"] | to_entries | .[] | .key + "=" + .value'

}


function main (){
    secret_id="$1"
    secret_type="$2"
    secret_file_name="$3"

    if [ "$secret_type" == "file" ] && [ ! -z "$secret_file_name" ]; then
	yav_secret_file_get "$secret_id" "$secret_file_name"
    elif [ "$secret_type" == "var" ]; then
	yav_secret_var_get "$secret_id"
    else
	echo "Houston, we have a problem!"
	exit 1
    fi
}


if [ $# -le 1 ]
then
    help_print
elif [ "$1" == "help" ]
then
    help_print
else
    main "$1" "$2" "$3"
fi

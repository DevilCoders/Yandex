#!/bin/bash
#set -x #echo on

function select_profile() {
    if [[ -z "$PROFILE" ]] || ([[ "preprod" != "$PROFILE" ]] && [[ "prod" != "$PROFILE" ]]); then
        PS3='Please enter profile: '
        options=("preprod" "prod" "quit")
        select opt in "${options[@]}"
        do
            case ${opt} in
                "preprod")
                    PROFILE="preprod"
                    break
                    ;;
                "prod")
                    PROFILE="prod"
                    break
                    ;;
                "quit")
                    exit 0
                    ;;
                *) echo "invalid option $REPLY";;
            esac
        done
    fi
    if [[ ! "$SILENT" == "true" ]]; then
        (>&2 echo "profile - $PROFILE")
    fi
}

select_profile

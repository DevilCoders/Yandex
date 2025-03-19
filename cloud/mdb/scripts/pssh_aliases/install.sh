#!/usr/bin/env bash

[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x

while [[ $# -gt 0 ]]; do
    case "$1" in
        -a|--auto)   auto=1; shift;;
        -h|--help)   help=1; shift; break;;
        --)          shift; break;;
        -*)          help=1; error=1; break;;
        *)           break;;
    esac
done

if [[ "$help" == 1 ]]; then
    cat <<EOF
Usage: $(basename $0) [<option>] ...
  -a, --auto         Suppress questions and perform installation in automatic mode.
  -h, --help         Show this help message and exit.
EOF
    exit ${error:0}
fi


system=$(uname -s | tr '[:upper:]' '[:lower:]')
case $system in
    darwin|linux)
        ;;
    *)
        echo "${system} system is not supported."
        exit 1
        ;;
esac


shell_name=$(basename "${SHELL}")
case ${shell_name} in
    bash)
        default_rc_path="${HOME}/.bashrc"
        if [ "${system}" = "darwin" ]; then
            default_rc_path="${HOME}/.bash_profile"
        fi
        ;;
    zsh)
        default_rc_path="${HOME}/.zshrc"
        ;;
    *)
        echo "${shell_name} shell is not supported."
        exit 1
        ;;
esac


function input_yes_no() {
    while read answer; do
        case "${answer}" in
        "Yes" | "y" | "yes" | "")
            return 0
            ;;
        "No" | "n" | "no")
            return 1
            ;;
        *)
            echo "Please enter 'y' or 'n': "
            ;;
        esac
    done
}


install_path="${HOME}/.pssh_aliases"
if [[ -n "$auto" ]]; then
    echo -e "PSSH aliases will be installed into ${install_path}\n"
else
    echo "PSSH aliases will be installed into ${install_path} [press enter to continue]"
    read
fi


if [[ -n "$auto" ]]; then
    rc_path="${default_rc_path}"
else
    echo -n "Enter a path to an rc file to update, or leave blank to use [${default_rc_path}]: "
    read filepath
    if [ "${filepath}" = "" ]; then
        filepath="${default_rc_path}"
    fi
    rc_path="$filepath"
    echo
fi


cat "$(dirname $0)/pssh_aliases.bash" > "${install_path}"

if ! grep -Fq "if [ -f '${install_path}' ]; then source '${install_path}'; fi" "${rc_path}"; then
    cat >> "${rc_path}" <<EOF

# The next line include pssh aliases.
if [ -f '${install_path}' ]; then source '${install_path}'; fi
EOF
    echo -e "PSSH aliases has been added to your '${rc_path}' profile."
fi


echo -e "To complete installation, start a new shell or type 'source \"${rc_path}\"' in the current one."

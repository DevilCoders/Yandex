#!/usr/bin/env bash

# Enable debug mode if requested through DEBUG environment variable.
[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x


# Utility functions.
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


# Parse command-line arguments.
tool=all
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--tool)
            case $2 in
                dbaas|mdb-admin|all)
                    tool=$2
                    ;;
                *)
                    echo "\"$2\" is an invalid value for --tool. Possible values: \"dbaas\", \"mdb-admin\" and \"all\"."
                    exit 1
                    ;;
            esac
            shift 2
            ;;
        -a|--auto)
            auto=1
            shift
            ;;
        -p|--path)
            install_path=$2
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

if [[ "$help" == 1 ]]; then
    cat <<EOF
Usage: `basename $0` [<option>] ...
  -t, --tool <value> Tool to install. Possible values: "dbaas", "mdb-admin" and "all". Default is "all".
  -p, --path <value> Install path. Can be set using environment variable DBAAS_TOOL_ROOT_PATH. Default is "~/mdb-scripts".
  -a, --auto         Suppress questions and perform installation in automatic mode.
  -h, --help         Show this help message and exit.
EOF
    exit ${error:0}
fi


# Check environment.
if ! command -v s3cmd >/dev/null; then
    echo "s3cmd was not found."
    exit 1
fi

system=$(uname -s | tr '[:upper:]' '[:lower:]')
case $system in
    linux|darwin)
        ;;
    *)
        echo "${system} system is not supported."
        exit 1
        ;;
esac

shell_name=$(basename "${SHELL}")
case ${shell_name} in
    bash)
        if [ "${system}" = "darwin" ]; then
            default_rc_path="${HOME}/.bash_profile"
        else
            default_rc_path="${HOME}/.bashrc"
        fi
        ;;
    zsh)
        default_rc_path="${HOME}/.zshrc"
        ;;
    fish)
        default_rc_path="${HOME}/.config/fish/config.fish"
        ;;
    *)
        echo "${shell_name} shell is not supported."
        exit 1
        ;;
esac


# Determine install path.
if [ -z "$install_path" ]; then
    if [ -n "$DBAAS_TOOL_ROOT_PATH" ]; then
        install_path="${DBAAS_TOOL_ROOT_PATH}"
    else
        install_path="${HOME}/mdb-scripts"
    fi
fi


# Download and install tools.
if [[ -n "$auto" ]]; then
    echo -e "CLI tools will be installed into ${install_path}\n"
else
    echo "CLI tools will be installed into ${install_path} [press enter to continue]"
    read
fi

s3_access_key=$(ya vault get version sec-01efpjrrbw9jc6vhf7f2qra6en -o access_key) &&
s3_secret_key=$(ya vault get version sec-01efpjrrbw9jc6vhf7f2qra6en -o secret_key)
if [[ $? -ne 0 ]]; then
    echo "Failed to get access secrets using 'ya vault'"
    exit 1
fi

mkdir -p "${install_path}/bin"
s3cmd="s3cmd --host storage.yandexcloud.net --host-bucket %(bucket)s.storage.yandexcloud.net --region ru-central1 --access_key $s3_access_key --secret_key $s3_secret_key"
version=$($s3cmd -q get s3://mdb-scripts/current_version -)
if [[ "$tool" == "dbaas" ]] || [[ "$tool" == "all" ]]; then
    $s3cmd get -f "s3://mdb-scripts/${version}/${system}/dbaas" "${install_path}/dbaas" || exit $?
    chmod +x "${install_path}/dbaas"
    mv "${install_path}/dbaas" "${install_path}/bin/dbaas"
fi
if [[ "$tool" == "mdb-admin" ]] || [[ "$tool" == "all" ]]; then
    $s3cmd get -f "s3://mdb-scripts/${version}/${system}/mdb-admin" "${install_path}/mdb-admin" || exit $?
    chmod +x "${install_path}/mdb-admin"
    mv "${install_path}/mdb-admin" "${install_path}/bin/mdb-admin"
fi
echo "$version" > "${install_path}/bin/version.txt"
echo


# Configure PATH and command completion.
if [[ -n "$auto" ]]; then
    rc_path="${default_rc_path}"
else
    echo -n "Modify profile to update your \$PATH and enable shell command completion? [Y/n] "

    if ! input_yes_no ; then
        exit 0
    fi

    echo -n "Enter a path to an rc file to update, or leave blank to use [${default_rc_path}]: "
    read filepath
    if [ "${filepath}" = "" ]; then
        filepath="${default_rc_path}"
    fi
    rc_path="$filepath"
    echo
fi

if [[ "${shell_name}" == "fish" ]]; then
    if ! grep -Fq "fish_add_path '${install_path}/bin'" "${rc_path}"; then
        cat >> "${rc_path}" <<EOF

# The next line updates PATH for mdb cli tools.
fish_add_path '${install_path}/bin'
EOF
        echo -e "\nmdb cli tools PATH has been added to your '${rc_path}' profile."
    fi
else
    path_file="${install_path}/path.${shell_name}.inc"
    echo "export PATH=\"${install_path}/bin:\${PATH}\"" > "${path_file}"

    if ! grep -Fq "if [ -f '${path_file}' ]; then source '${path_file}'; fi" "${rc_path}"; then
        cat >> "${rc_path}" <<EOF

# The next line updates PATH for mdb cli tools.
if [ -f '${path_file}' ]; then source '${path_file}'; fi
EOF
        echo -e "\nmdb cli tools PATH has been added to your '${rc_path}' profile."
    fi
fi

if [[ "${shell_name}" == "fish" ]]; then
    if [[ "$tool" == "dbaas" ]] || [[ "$tool" == "all" ]]; then
        env _DBAAS_COMPLETE=fish_source ${install_path}/bin/dbaas > "${HOME}/.config/fish/completions/dbaas.fish"
    fi
    if [[ "$tool" == "mdb-admin" ]] || [[ "$tool" == "all" ]]; then
        mdb-admin completion fish >> "${HOME}/.config/fish/completions/mdb-admin.fish"
    fi
else
    shell_completion_file="${install_path}/completion.${shell_name}.inc"
    echo > "${shell_completion_file}"
    if [[ "$tool" == "dbaas" ]] || [[ "$tool" == "all" ]]; then
        env _DBAAS_COMPLETE=${shell_name}_source ${install_path}/bin/dbaas >> "${shell_completion_file}"
    fi
    if [[ "$tool" == "mdb-admin" ]] || [[ "$tool" == "all" ]]; then
        mdb-admin completion "${shell_name}" >> "${shell_completion_file}"
    fi

    if ! grep -Fq "if [ -f '${shell_completion_file}' ]; then source '${shell_completion_file}'; fi" "${rc_path}"; then
        cat >> "${rc_path}" <<EOF

# The next line enables shell command completion for mdb cli tools.
if [ -f '${shell_completion_file}' ]; then source '${shell_completion_file}'; fi
EOF
        echo -e "\n${shell_name} completion for mdb cli tools has been installed and added to your '${rc_path}' profile."
    fi
fi


echo -e "To complete installation, start a new shell or type 'source \"${rc_path}\"' in the current one."

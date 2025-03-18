#!/usr/bin/env bash
export PATH=$HOME/.local/bin:$PATH

if [ "`id -u`" -ne "0" ]; then
    echo "Seems that you're not root. Ensure you're running this script with '--user' flag. "
fi

pip_cmd="pip3"
if ! which "${pip_cmd}"; then
    echo "pip3 not found, switching to pip"
    pip_cmd="pip"
fi

echo "Upgrading pip"
${pip_cmd} install --upgrade pip "$@"

echo "Current pip:"

which ${pip_cmd}

echo "Installing main requirements"
${pip_cmd} install -r requirements.txt --upgrade "$@"

echo "Installing yandex-specific requirements"
${pip_cmd} install --index http://pypi.yandex-team.ru/simple/ --trusted-host pypi.yandex-team.ru -r requirements-yandex.txt --upgrade "$@"

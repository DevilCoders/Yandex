#!/usr/bin/env bash

SYSTEM=$(uname -s)

echo ""
echo "================================="
echo "Support tools installer & updater"
echo "================================="
echo ""
echo "Detecting your OS type..."

if [ "${SYSTEM}" = "Darwin" ]; then
	Q_URL=https://github.yandex-team.ru/akimrx/quota-manager/releases/download/stable/quotas-osx
	S_URL=https://github.yandex-team.ru/akimrx/quota-manager/releases/download/stable/saint-osx
elif [ "${SYSTEM}" = "Linux" ]; then
	Q_URL=https://github.yandex-team.ru/akimrx/quota-manager/releases/download/stable/quotas-linux
	S_URL=https://github.yandex-team.ru/akimrx/quota-manager/releases/download/stable/saint-linux
fi

UPDATE_URL=https://github.yandex-team.ru/akimrx/quota-manager/releases/download/stable/quotas-update.sh

echo "OS: ${SYSTEM}"
echo ""
echo "Checkout and downloading quotactl..."

mkdir -p ~/usr/local/bin; wget ${Q_URL} -q --show-progress --tries=3 -O ~/usr/local/bin/quotactl; chmod 775 ~/usr/local/bin/quotactl

echo""
echo "Checkout and downloading updater..."

wget ${UPDATE_URL} -q --show-progress --tries=3 -O ~/usr/local/bin/quotactl-update; chmod 775 ~/usr/local/bin/quotactl-update

echo ""
echo "Checkout and downloading saint..."

wget ${S_URL} -q --show-progress --tries=3 -O ~/usr/local/bin/saint; chmod 775 ~/usr/local/bin/saint

SHELL_NAME=$(basename "${SHELL}")
RC_PATH="${HOME}/.bashrc"

if [ "${SHELL_NAME}" != "bash" ]; then
	RC_PATH="${HOME}/.${SHELL_NAME}rc"
elif [ "${SYSTEM}" = "Darwin" ]; then
        RC_PATH="${HOME}/.bash_profile"
elif [ "${SHELL_NAME}" = "zsh" ]; then
	RC_PATH="${HOME}/.zshrc"
fi

echo ""
echo "Checkout PATH in ${RC_PATH}"

if grep -q 'export PATH=$HOME/usr/local/bin:$PATH' "${RC_PATH}"; then
	echo "PATH already added. Nothing to do."
else
	echo "Added PATH to ${RC_PATH}"; echo 'export PATH=$HOME/usr/local/bin:$PATH' >> "${RC_PATH}"; 
	echo "Please, reload your shell and run: quotactl --init or saint --init"
fi


if grep -q 'alias qctl="quotactl"' "${RC_PATH}"; then
	echo ""
else
	echo 'alias qctl="quotactl"' >> "${RC_PATH}"
fi


if grep -q 'alias saint-update="quotactl-update"' "${RC_PATH}"; then
	echo ""
else
	echo 'alias saint-update="quotactl-update"' >> "${RC_PATH}"
fi


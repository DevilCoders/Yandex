#!/bin/sh -e

if [ ! -f ${HOME}/.pypirc ]; then
    echo "
1. Go to https://pypi.yandex-team.ru/accounts/access-keys/
2. Create new Access key (click [Create new] button)
3. Write in ~/.pypirc this lines

 [distutils]
 Index-servers =
     yandex

 [yandex]
 Repository: https://pypi.yandex-team.ru/simple/
 Username: <access-key>
 Password: <secret-key>

4. chmod 0400 ~/.pypirc
"
    exit 1
fi

ya make -tA
python setup.py sdist upload -r yandex

echo "Check that the new version is uploaded at https://pypi.yandex-team.ru/dashboard/repositories/default/packages/solomon/"

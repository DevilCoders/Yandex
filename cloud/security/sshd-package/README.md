# SSHD Configuration for Bastion enforcement at Yandex Cloud.

Before building add new line to changelog using `dch -i`

## Files:
`sshd/01-breakglass` - sudoers file for breakglass access
`sshd/sshca` - contains root CAs for ssh-pki used at Yandex Cloud
`sshd/sshd-cloud` - sshd configuration file

For more information please visit:

https://wiki.yandex-team.ru/cloud/security/research/cloud-sshd/

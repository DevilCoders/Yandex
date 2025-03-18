#!/usr/bin/env bash
set -euo pipefail

# Скрипт правит .profile на production машинах Антиробота, чтобы был более простой доступ до нужных папок.

TAG=G@ALL_ANTIROBOT
BASH_PROFILE_ANTIROBOT=.bash_profile.antirobot

cat << _EOF > $BASH_PROFILE_ANTIROBOT
[ -L ~/antirobot ] && rm ~/antirobot
ln -s /db/iss3/services/13512/active/production_antirobot_* ~/antirobot
PATH=\$PATH:~/antirobot

[ ! -L ~/logs ]    && ln -s /db/www/logs ~/logs
[ ! -L ~/runtime ] && ln -s /db/bsconfig/webcache ~/runtime
[ ! -d /var/tmp/\$USER ] && mkdir -p /var/tmp/\$USER
[ ! -e ~/tmp ] && ln -s /var/tmp/\$USER ~/tmp

TAG=$TAG
_EOF

RBTORRENT=$(sky share $BASH_PROFILE_ANTIROBOT)

sky run -Up --cqudp 'cd ~ && sky get -wu '$RBTORRENT' && (fgrep --quiet '$BASH_PROFILE_ANTIROBOT' .profile || echo -e "\n. \$HOME/'$BASH_PROFILE_ANTIROBOT'" >> .profile)' $TAG

rm $BASH_PROFILE_ANTIROBOT

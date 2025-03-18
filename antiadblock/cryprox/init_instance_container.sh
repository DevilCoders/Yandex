#keys_placement
echo 'keys_placement init start'
mkdir -p /logs/nginx
cp -R ./cookiematcher_crypt_keys /etc/cookiematcher
cp -R ./nginx_keys /etc/nginx/keys
echo 'keys_placement init finished'

#fill_crontab
printf "* * * * * logrotate -s /logs/logrotate.state /etc/logrotate/logrotate.conf\n*/5 * * * * /usr/bin/update_resources.py > /logs/update_resources.log 2>&1\n" | crontab
echo 'fill_crontab finished'

#mount_tmpfs
echo 'start mount_tmpfs'
if [ ! -e /tmp_uids ]; then
  echo 'creating tmp_uids'
  mkdir -p /tmp_uids
  echo 'tmp_uids created'
  portoctl vcreate /tmp_uids backend=tmpfs space_limit=5Gb containers=$(portoctl get self parent)
  echo 'vcreate completed'
  df -h
fi
echo 'finished mount_tmpfs'

# https://st.yandex-team.ru/DEPLOY-3378
echo 'start fix_mtu'
export DEF_RTR=`ip -o -6 route get 2a02:6b8::1:1 | cut -d ' ' -f 5`
ip -6 route replace 2a02:6b8::/32 via $DEF_RTR dev veth mtu 8910
ip -6 route replace 2620:10f:d000::/44 via $DEF_RTR dev veth mtu 8910
ip -6 route replace default via $DEF_RTR dev veth mtu 1450 advmss 1390
echo 'finished fix_mtu'

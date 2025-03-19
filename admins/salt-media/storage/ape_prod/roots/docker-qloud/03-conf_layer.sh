git clone https://github.yandex-team.ru/salt-media/storage.git
#d /storage
#git checkout `cat /build/env`
cd /
mkdir /build/salt

cat > /etc/resolv.conf << EOF
search search.yandex.net yandex.ru
nameserver fd53::1
nameserver 2a02:6b8::1:1
options timeout:1 attempts:1
EOF

cat > /build/salt/minion << EOF
file_client: local
fileserver_backend:
  - roots

file_roots:
  stable:
    - /storage/ape_prod/roots
    - /storage/common/roots
  testing:
    - /storage/ape/roots
    - /storage/common/roots

pillar_roots:
  stable:
    - /storage/ape_prod/pillar
    - /storage/common/pillar
  testing:
    - /storage/ape/pillar
    - /storage/common/pillar

ipv6: True
log_level: info
EOF

echo "apply cocaine state"
#remove templates yasmagent and other
if [ `cat /build/env` == "testing" ]
then
    echo 'detected testing project'
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape/roots/`cat /build/main_sls`.sls
fi
if [ `cat /build/env` == "stable" ]
then
    echo 'detected stable project'
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
    sed -i '$d' /storage/ape_prod/roots/`cat /build/main_sls`.sls
fi

salt-call -l info state.apply `cat /build/main_sls` --retcode-passthrough -c /build/salt saltenv=`cat /build/env` || true
echo "apply qloud state"
salt-call -l info state.apply `cat /build/qloud_sls` --retcode-passthrough -c /build/salt saltenv=`cat /build/env` || true

rm -rf /build
rm -rf /storage

echo "Application config layer created on $(date -u +%F@%T) UTC" >>/etc/porto_image.info

mkdir /DB
sudo mkfs.ext4 -b 4096 /dev/vdb

# add /dev/vdb to fstab
blk=(`blkid | grep /dev/vdb`)
uuid=${blk[1]}
echo "${uuid} /DB ext4 errors=remount-ro 0       1" >> /etc/fstab
mount /dev/vdb /DB

service postgresql stop
cp -R -p  /var/lib/postgresql /DB/postgresql
sed 's/\/var\/lib\/postgresql/\/DB\/postgresql/' /etc/postgresql/9.5/main/postgresql.conf -i
sed 's/peer/trust/' /etc/postgresql/9.5/main/pg_hba.conf -i
service postgresql start

sudo -u postgres -i psql -c "CREATE DATABASE pgbench;"


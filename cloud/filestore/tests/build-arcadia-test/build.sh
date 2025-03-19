set -ex

# install arc
sudo apt update
sudo apt -y install python3 python-is-python3
wget https://get.arc-vcs.yandex.net/launcher/linux -O /usr/bin/arc && chmod +x /usr/bin/arc
arc --update

# setup building layout
mkdir $mountPath/.ya;
mkdir $mountPath/arc_store
ln -s $mountPath/.ya /root/.ya;

# mount arcadia
export LOGNAME="$user"
arc token store
arc mount -m $arcadiaPath -S $mountPath/arc_store

# run build
cd $arcadiaPath;
$yaPath make $buildPath -v

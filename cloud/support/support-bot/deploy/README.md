# Just in case you forgot how to use it

## Prepare env

### Install YC CLI
* Download CLI:  
```bash
curl https://storage.yandexcloud.net/yandexcloud-yc/install.sh | bash
```

* Create SA key:  
```bash
yc config set service-account-key ~/.config/yandex-cloud/anykey.json
```

### Install Docker
```bash
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu bionic stable"
sudo apt update
sudo apt install docker-ce
sudo systemctl status docker
sudo usermod -aG docker <username>
sudo systemctl enable docker

yc container registry  configure-docker
```

### Enable IPv6 for Docker
* Edit `sudo vim /etc/docker/daemon.json` :  
```json
{
    "ipv6": true,
    "fixed-cidr-v6": "fd00::/80"
}
```

* Enable masquerdae:  
```bash
sudo apt install iptables-persistent netfilter-persistent -y
sudo systemctl start netfilter-persistent
sudo systemctl enable netfilter-persistent
sudo iptables -P FORWARD ACCEPT
sudo ip6tables -t nat -A POSTROUTING -s fd00::/80 ! -o docker0 -j MASQUERADE
sudo netfilter-persistent save
```

* Restart and checkout:  
```bash
sudo systemctl reload docker
sudo systemctl restart docker
docker network inspect bridge
```

* If not works, try:  
```bash
sudo ip -6 route add 2001:db8:1::/64 dev docker0
sudo sysctl net.ipv6.conf.default.forwarding=1
sudo sysctl net.ipv6.conf.all.forwarding=1
```

* [Official documentation](https://docs.docker.com/config/daemon/ipv6/)  
* [Useful link](https://medium.com/@skleeschulte/how-to-enable-ipv6-for-docker-containers-on-ubuntu-18-04-c68394a219a2)


### Build image, push & pull

* On local machine build image and push it to CR:  
```bash
docker build . -t cr.yandex/crpo08163079v7fhg914/yc-support-bot-prod:<tag> --build-arg USERNAME=<username> --build-arg GITPASS=<password> --no-cache
docker push cr.yandex/crpo08163079v7fhg914yc-support-bot-prod:<tag>
```

* On server pull image and create container:  
```bash
docker pull cr.yandex/crpo08163079v7fhg914/yc-support-bot-prod:<tag>
docker container create --name yc-support-bot --network host --hostname yc-support-bot --tty --restart no cr.yandex/crpo08163079v7fhg914/yc-support-bot-prod:<tag>
docker container start yc-support-bot
docker container logs yc-support-bot
```

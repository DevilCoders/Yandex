# How to install docker
1. sudo apt-get install docker.io
2. sudo usermod -a -G docker <login>
3. reboot notebook
4. acquire token as described in 
https://wiki.yandex-team.ru/docker-registry/#authorization

# Deploy new Docker image
ya package --docker --docker-repository=yandex-cloud --docker-push --docker-network=host ./pkg.json

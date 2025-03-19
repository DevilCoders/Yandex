#!/bin/bash

# wait for the internet (? seems to be packer problem)
sleep 120

echo 'install docker'
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker ubuntu

echo 'install nvidia-docker'
echo 'Download nvidia-runtime packages'
curl -s -L https://nvidia.github.io/nvidia-container-runtime/gpgkey | \
  sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-container-runtime/ubuntu16.04/amd64/nvidia-container-runtime.list | \
  sudo tee /etc/apt/sources.list.d/nvidia-container-runtime.list
sudo apt-get update

echo 'Install nvidia-runtime'
sudo apt-get install -y nvidia-container-runtime

echo 'Docker Engine setup'
sudo mkdir -p /etc/systemd/system/docker.service.d
sudo tee /etc/systemd/system/docker.service.d/override.conf <<EOF
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd --host=fd:// --add-runtime=nvidia=/usr/bin/nvidia-container-runtime
EOF
sudo systemctl daemon-reload
sudo systemctl restart docker
sudo systemctl status docker

echo 'Add the package repositories'
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | \
  sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/ubuntu16.04/amd64/nvidia-docker.list | \
  sudo tee /etc/apt/sources.list.d/nvidia-docker.list
sudo apt-get update

echo 'Install nvidia-docker2 and reload the Docker daemon configuration'
sudo apt-get install -y nvidia-docker
sudo systemctl restart docker
sudo systemctl status docker

echo 'install anaconda'
wget -q https://repo.anaconda.com/archive/Anaconda3-2020.07-Linux-x86_64.sh -O $HOME/anaconda.sh
bash $HOME/anaconda.sh -b -p $HOME/anaconda
eval "$($HOME/anaconda/bin/conda shell.bash hook)"

echo 'install flask'
pip install flask

echo 'install yt'
pip install -i https://pypi.yandex-team.ru/simple/ yandex-yt

WORKDIR="$HOME"
cd $WORKDIR

echo 'download code of nv-launcher-agent'
sandbox_resource_id="1731597153"
mkdir nv-launcher-agent && cd nv-launcher-agent
wget -q https://proxy.sandbox.yandex-team.ru/$sandbox_resource_id -O resource.tar
tar -xvf resource.tar

echo 'starting nv-launcher-agent service'
sudo cp nv-launcher-agent.service /etc/systemd/system/
sudo chmod 664 /etc/systemd/system/nv-launcher-agent.service
sudo systemctl daemon-reload
sudo systemctl enable nv-launcher-agent
sudo systemctl start  nv-launcher-agent

# echo 'delete trash'
sudo apt-get purge -y network-manager network-manager-pptp


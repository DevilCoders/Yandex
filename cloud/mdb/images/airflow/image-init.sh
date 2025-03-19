#!/usr/bin/env bash
set -ex

# This script is being run once during image creation

if [[ -z "${AIRFLOW_MAJOR_VERSION}" ]]; then
  AIRFLOW_MAJOR_VERSION="2"
fi
echo "AIRFLOW_MAJOR_VERSION=$AIRFLOW_MAJOR_VERSION"

apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install \
python-pip-whl=20.0.2-5ubuntu1 \
python3-pip=20.0.2-5ubuntu1 \
postgresql=12+214ubuntu0.1 \
postgresql-contrib=12+214ubuntu0.1 \
-yq

pip3 install apache-airflow[postgres,yandex]==2.2.3 --constraint "/tmp/image-bootstrap/pip-airflow-constraints.txt"
# Yandex provider version is fixed to 2.1.0 in constraint file
pip3 install apache-airflow-providers-yandex==2.2.0 yandexcloud==0.173.0

sudo -u postgres psql -c "CREATE USER airflow PASSWORD 'airflow'"
sudo -u postgres psql -c "CREATE DATABASE airflow"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO airflow"

groupadd airflow
useradd -m airflow --gid airflow
usermod -a -G airflow airflow

AIRFLOW_HOME="/etc/airflow"
mkdir "${AIRFLOW_HOME}"
echo "export AIRFLOW_HOME=${AIRFLOW_HOME}" >> /etc/profile

mkdir /var/log/airflow && chown airflow:airflow /var/log/airflow
mkdir /var/local/airflow && chown airflow:airflow /var/local/airflow
mkdir /home/airflow/dags && chown airflow:airflow /home/airflow/dags
mkdir /home/airflow/plugins && chown airflow:airflow /home/airflow/plugins

mv /tmp/image-bootstrap/airflow-init.service /etc/systemd/system/airflow-init.service
mv /tmp/image-bootstrap/airflow-webserver.service /etc/systemd/system/airflow-webserver.service
mv /tmp/image-bootstrap/airflow-scheduler.service /etc/systemd/system/airflow-scheduler.service
mv /tmp/image-bootstrap/airflow.cfg ${AIRFLOW_HOME}/airflow.cfg
mv /tmp/image-bootstrap/airflow-init.sh ${AIRFLOW_HOME}/airflow-init.sh && chmod +x ${AIRFLOW_HOME}/airflow-init.sh
chown -R airflow:airflow ${AIRFLOW_HOME}

# Adding useful information to SSH welcome message
mv /tmp/image-bootstrap/10-airflow-help /etc/update-motd.d/10-airflow-help
chmod +x /etc/update-motd.d/10-airflow-help
rm -f /etc/update-motd.d/10-help-text
rm -f /etc/update-motd.d/50-motd-news
rm -f /etc/update-motd.d/91-release-upgrade
rm -f /etc/update-motd.d/92-unattended-upgrades
systemctl disable motd-news.service  # this service would be so sad without its beloved /etc/update-motd.d/50-motd-news
systemctl disable motd-news.timer

chmod +x /tmp/image-bootstrap/cleanup.sh
/tmp/image-bootstrap/cleanup.sh

rm -rf /tmp/image-bootstrap

systemctl enable airflow-init
systemctl enable airflow-scheduler
systemctl enable airflow-webserver

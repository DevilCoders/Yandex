FROM ubuntu:trusty
RUN  apt-get -qq update && apt-get -y install python3-pip curl libyaml-dev
COPY src /opt/
RUN  curl -ksfo /tmp/yaconfig.zip https://github.yandex-team.ru/_codeload/admins/yaconfig/zip/master && pip3 install /tmp/yaconfig.zip && rm /tmp/yaconfig.zip
RUN  pip3 install --upgrade -r /opt/requirements.txt
RUN  cd /opt && pip3 install .

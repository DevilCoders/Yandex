FROM ubuntu:18.04
MAINTAINER Dmitriy Sarafannikov <dsarafan@yandex-team.ru>

RUN apt-get update && apt-get install -y gnupg curl locales openssh-server supervisor

ADD pgdg.list /etc/apt/sources.list.d/
ADD yandex-mdb-bionic.list /etc/apt/sources.list.d/
ADD dist.key /tmp/
ADD pgdg.pref /etc/apt/preferences.d/pgdg.pref

RUN apt-key add /tmp/dist.key
RUN curl https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add -

RUN apt-get update

# Set Moscow timezone

ENV TZ=Europe/Moscow
RUN ln -sf /usr/share/zoneinfo/$TZ /etc/localtime
RUN echo $TZ > /etc/timezone

# Brave new Unicode world!

RUN locale-gen en_US en_US.UTF-8
RUN dpkg-reconfigure --frontend=noninteractive locales

# In some cases (I found Postgre postinst)
# Ubuntu `help us`
# ....
# > Determine and set system's default locale; we do not want to trust the
# > environment here, as ssh and sudo both propagate the user's locale from
# > potentially a remote host, and that might not even exist; also, we want to be
# > predictable.

# Hower for Python 2 `perfect unicode support`
ENV LANG=en_US.utf8 LANGUAGE=en_US:en LC_ALL=en_US.UTF-8

RUN echo "LANG=$LANG\nLANGUAGE=$LANGUAGE\nLC_ALL=$LC_ALL" > /etc/default/locale

RUN mkdir /root/.ssh && chmod 700 /root/.ssh
COPY test_ssh_key.pub /root/.ssh/authorized_keys
COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY ssh.conf /etc/supervisor/conf.d/ssh.conf

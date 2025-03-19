FROM python:3.8-slim
LABEL maintainer="akimrx@yandex-team.ru"
ARG USERNAME
ARG GITPASS

ENV DEBIAN_FRONTEND noninteractive

# Installing tools
RUN apt-get -qq update
RUN apt-get install -yqq git wget vim
RUN apt-get install -yqq tzdata

# Cloning app from github
WORKDIR /usr/src/
RUN mkdir yc-support-bot
COPY ./ /usr/src/yc-support-bot/

# Install submodules
WORKDIR /usr/src/yc-support-bot
RUN pip install -i https://pypi.yandex-team.ru/simple/ startrek_client
RUN pip install -i https://pypi.yandex-team.ru/simple/ tvmauth
RUN pip install -r requirements.txt
# RUN git checkout dev

# Set timezone
ENV TZ=Europe/Moscow
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN dpkg-reconfigure -f noninteractive tzdata

# Copy CA & config
COPY root.crt /root/.support-bot/root.crt
COPY config.yaml /root/.support-bot/config.yaml

# Fix language
ENV PYTHONIOENCODING utf-8

# Run app
#CMD ["python3", "main.py"]

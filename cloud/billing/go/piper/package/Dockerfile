FROM cr.yandex/mirror/ubuntu:20.04

# By default ubuntu:20.04 image has no any certificates, so we have to install it
RUN export DEBIAN_FRONTEND="noninteractive"
RUN echo 'deb http://yandex-cloud-secure.dist.yandex.ru/yandex-cloud-secure stable/all/'   |  tee -a /etc/apt/sources.list && \
    echo 'deb http://yandex-cloud-secure.dist.yandex.ru/yandex-cloud-secure stable/amd64/' |  tee -a /etc/apt/sources.list && \
    echo 'deb http://yandex-cloud-upstream-focal-secure.dist.yandex.ru/yandex-cloud-upstream-focal-secure stable/all/'   | tee -a /etc/apt/sources.list && \
    echo 'deb http://yandex-cloud-upstream-focal-secure.dist.yandex.ru/yandex-cloud-upstream-focal-secure stable/amd64/' | tee -a /etc/apt/sources.list && \
    apt update -o Acquire::AllowInsecureRepositories=true && \
    apt upgrade -y --allow-unauthenticated && \
    apt install yandex-internal-root-ca -y --allow-unauthenticated && \
    apt install ca-certificates -y --allow-unauthenticated && \
    apt-get clean && \
    update-ca-certificates

COPY cmd/*/* /usr/bin/
COPY version.txt /version.txt

ENTRYPOINT [ "/usr/bin/piper", "-c", "/config/piper/*.yaml"]

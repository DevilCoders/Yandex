ARG IMG_REGISTRY=cr.yandex/crpjcb1mr2vhfkktu5ss
FROM ${IMG_REGISTRY}/datacloud-base:latest

RUN apt update && apt install aptly
COPY build/dist-sync /opt/yandex/dist-sync
RUN useradd -ms /bin/bash dist-sync

# import dist key
RUN curl -s http://dist.yandex.ru/REPO.asc | gpg --no-tty --no-default-keyring --keyring trustedkeys.gpg --import

USER dist-sync
CMD ["/opt/yandex/dict-sync"]


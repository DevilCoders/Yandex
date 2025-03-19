FROM uo0ya/combainer:v2020-11-03T00.00.00

RUN echo -e "\
# ==yandex== \n\
# common\n\
deb http://common.dist.yandex.ru/common stable/all/\n\
deb http://common.dist.yandex.ru/common stable/amd64/\n\
deb http://common.dist.yandex.ru/common testing/all/\n\
deb http://common.dist.yandex.ru/common testing/amd64/\n\
# distro\n\
deb http://bionic.dist.yandex.ru/yandex-bionic/ stable/amd64/\n\
# search\n\
deb http://search-bionic.dist.yandex.ru/search-bionic/ stable/amd64/\n\
deb http://search-bionic.dist.yandex.ru/search-bionic/ stable/all/\n\
" >> /etc/apt/sources.list

RUN apt-get update --allow-insecure-repositories --allow-unauthenticated \
    && apt-get dist-upgrade -y --force-yes --allow-unauthenticated \
    && apt-get -y install --no-install-recommends --allow-unauthenticated \
        # yandex-porto
        yandex-porto/stable python3-portopy/stable python3-protobuf \
        media-graphite-sender \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.utf8 ru_RU.utf8
RUN ln -vsTf /usr/share/zoneinfo/Europe/Moscow /etc/localtime \
    && echo Europe/Moscow > /etc/timezone

COPY loggiver               /etc/yandex/loggiver/
RUN echo "www-data ALL=NOPASSWD: /etc/yandex/loggiver/pattern.d/app_resources.py" \
    > /etc/sudoers.d/loggiver_combainer_apps
RUN chmod 0440 /etc/sudoers.d/loggiver_combainer_apps

# cleanup origin plugins
RUN rm -vrf /usr/lib/combaine/custom /usr/lib/combaine/juggler /usr/lib/combaine/testing
COPY plugins                /usr/lib/combaine/

RUN cd /usr/lib/combaine/custom && ls -1 *.py | xargs -n1 cythonize -3 -i || true
RUN cd /usr/lib/combaine/testing/custom && ls -1 *.py | xargs --no-run-if-empty -n1 cythonize -3 -i || true

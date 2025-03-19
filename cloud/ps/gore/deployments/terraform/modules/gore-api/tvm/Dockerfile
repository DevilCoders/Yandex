FROM ubuntu
RUN echo 'deb http://dist.yandex.ru/yandex-bionic/ stable/amd64/' > /etc/apt/sources.list.d/yandex-dist.list \
 && apt-get update -o Acquire::https::Verify-Peer="false" -o Acquire::https::Verify-Host="false" -o Acquire::AllowInsecureRepositories="true" -qq \
 && apt-get install -y --allow-unauthenticated yandex-passport-tvmtool \
 && rm -rf /var/lib/apt/lists/*

ENTRYPOINT [ "/usr/sbin/tvmtool" ]
CMD [ "bbenv", "-e", "2", "-c", "/etc/tvmtool/tvmtool.conf" ]

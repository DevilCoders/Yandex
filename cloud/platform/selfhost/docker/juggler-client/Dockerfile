FROM cr.yandex/crp7nvlkttssi7kapoho/base:2019-03-28T16-38

MAINTAINER https://wiki.yandex-team.ru/sm/juggler/juggler-client/

RUN apt-get install --force-yes -y juggler-client yandex-juggler-http-check yandex-push-client
ADD juggler-client.default /etc/default/juggler-client
ADD client.conf /home/monitor/juggler/etc/client.conf
RUN mkdir -p /juggler-bundle && curl "https://proxy.sandbox.yandex-team.ru/1112386298" | tar -C /juggler-bundle -zxf -

CMD ["/usr/bin/juggler-client", "--config-dir", "/home/monitor/juggler/etc"]

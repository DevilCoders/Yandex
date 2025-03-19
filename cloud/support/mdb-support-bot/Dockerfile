FROM debian:buster-slim

RUN rm -f /etc/localtime; ln -s /usr/share/zoneinfo/Europe/Moscow /etc/localtime
#RUN echo nameserver 77.88.8.1 > /etc/resolv.conf

COPY root.crt /root/.postgresql/
COPY mdb-support-bot ./bot/

WORKDIR /bot

CMD ["./mdb-support-bot"]
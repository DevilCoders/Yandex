FROM debian:buster-slim

RUN rm -f /etc/localtime; ln -s /usr/share/zoneinfo/Europe/Moscow /etc/localtime

COPY root.crt /root/.postgresql/
COPY aws /.aws
COPY dutybot ./bot/

WORKDIR /bot

CMD ["./dutybot"]
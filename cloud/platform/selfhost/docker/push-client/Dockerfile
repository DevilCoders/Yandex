FROM cr.yandex/crp7nvlkttssi7kapoho/base:2019-03-28T16-38
RUN apt-get update && apt-get install --force-yes -y yandex-push-client=6.60.0
CMD ["/usr/bin/push-client", "-f", "-c", "/etc/yandex/statbox-push-client/push-client.yaml"]

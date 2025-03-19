FROM alpine

RUN apk --update add logrotate
RUN echo "*	*	*	*	*	/usr/sbin/logrotate /etc/logrotate.d/logrotate-app.conf" >> /etc/crontabs/root

# Do not forget mount config /etc/logrotate.d/logrotate_app.conf

CMD ["crond", "-f"]
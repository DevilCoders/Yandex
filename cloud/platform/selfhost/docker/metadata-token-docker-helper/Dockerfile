FROM docker:19.03.1
ENV DOCKER_CONFIG="/opt/ycr/.docker/"
RUN apk add curl && \
   curl "https://storage.yandexcloud.net/ycr-tools/ycr-tools/v9-db4514c01/linux/amd64/docker-credential-ycr" \
      -o /bin/docker-credential-ycr && \
   chmod a+x /bin/docker-credential-ycr && \
   docker-credential-ycr configure-docker
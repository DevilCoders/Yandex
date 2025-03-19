FROM debian
WORKDIR /app/
RUN  apt-get update -qq \
  && apt-get install -y curl \
  && rm -rf /var/lib/apt/lists/*
RUN mkdir -p /usr/local/share/ca-certificates/Yandex \
  && curl "https://storage.yandexcloud.net/cloud-certs/CA.pem" \
    --output /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt \
    --silent
RUN update-ca-certificates
COPY gore ./
CMD ["./gore"]

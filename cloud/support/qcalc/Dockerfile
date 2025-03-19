FROM ubuntu:latest

RUN apt update && apt upgrade -y && apt-get clean

RUN apt install wget -y && wget http://archive.ubuntu.com/ubuntu/pool/main/l/linux/linux-libc-dev_4.15.0-65.74_amd64.deb && \
dpkg -i ./linux-libc-dev_4.15.0-65.74_amd64.deb

RUN apt update && \
apt install openssl curl gnupg2 lsb-release ca-certificates python3 uwsgi uwsgi-plugin-python3 postgresql-server-dev-all gcc python3-dev musl-dev python3-pip -y && \
apt-get clean

RUN echo "deb http://nginx.org/packages/mainline/ubuntu `lsb_release -cs` nginx" | tee /etc/apt/sources.list.d/nginx.list && \
curl -fsSL https://nginx.org/keys/nginx_signing.key | apt-key add -

RUN apt update && apt -y install nginx && apt-get clean

RUN pip3 install flask requests psycopg2

COPY ./app/config/quotacalc_nginx.conf /etc/nginx/conf.d/default.conf
COPY ./app/config/quotacalc_uwsgi.conf /etc/uwsgi/apps-enabled/quotacalc.conf

WORKDIR /app

COPY ./ /app

RUN mkdir -p ~/.postgresql && \
wget "https://storage.yandexcloud.net/cloud-certs/CA.pem" -O ~/.postgresql/root.crt && \
chmod 0600 ~/.postgresql/root.crt && \
wget https://crls.yandex.net/allCAs.pem -O /app/app/static/allCAs.pem

ENTRYPOINT nginx -g 'daemon off;' & uwsgi --ini /etc/uwsgi/apps-enabled/quotacalc.conf --plugins python3
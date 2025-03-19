FROM registry.yandex.net/data-ui/node-nginx:12r1

RUN apt-get update && apt-get install -y make g++ zip unzip graphviz

RUN mkdir -p /opt/app
WORKDIR /opt/app

COPY package.json package-lock.json /opt/app/
RUN npm ci

COPY . .

RUN ls node_modules/@yandex-data-ui
RUN npm run build
RUN npm prune --production && \
  rm -rf /tmp/* && \
  chown app /opt/app/dist/run

COPY deploy/nginx /etc/nginx
COPY deploy/supervisor /etc/supervisor/conf.d

COPY server /opt/app/

RUN mkdir -p /opt/app/dist/public/build/speedscope && \
  wget https://github.com/jlfwong/speedscope/releases/download/v1.13.0/speedscope-1.13.0.zip -O /opt/app/speedscope.tar.gz && \
  unzip /opt/app/speedscope.tar.gz -d /opt/app/dist/public/build

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]

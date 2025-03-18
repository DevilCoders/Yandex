FROM registry.yandex.net/tools/nodejs:8

WORKDIR /app
EXPOSE 80

COPY .npmrc /app/
COPY package.json /app/
COPY package-lock.json /app/

RUN npm install
RUN npm run tanker-install

COPY . /app

ENV NODE_ENV production
RUN npm run i18n
RUN npm run build

CMD node --max-old-space-size=900 /app/index.js

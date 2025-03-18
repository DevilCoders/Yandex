#!/usr/bin/env bash

set -e

node_v_required="v12.18.0"
npm_v_required="6.14.4"

if [[ $(node -v) != "$node_v_required" ]]; then
    echo "node version $node_v_required required"
    exit 1
fi

if [[ $(npm -v) != "$npm_v_required" ]]; then
    echo "npm version $npm_v_required required"
    exit 1
fi

path=$(pwd)

echo "Installing frontend dependencies..."
cd `dirname $0`/../../../frontend
npm ci --registry=https://npm.yandex-team.ru

echo "Installing frontend/packages/external-captcha-themer dependencies..."
cd packages/external-captcha-themer
npm install --registry=https://npm.yandex-team.ru
npm ci --registry=https://npm.yandex-team.ru

echo "Building frontend/packages/external-captcha-themer"
npm run build

cd dist

echo "Please add following lines to ya.make:"
echo "# --- themer --- "
mv index.html themer.html
for p in $(ls *.js *.svg *.css *.html *.png); do
    cp $p $path/../themer_generated/$p
    echo "\${CURDIR}/themer_generated/$p"
done
echo "# --- /themer --- "

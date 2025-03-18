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

echo "Installing frontend/packages/external-captcha dependencies..."
cd packages/external-captcha
npm ci --registry=https://npm.yandex-team.ru

echo "Building frontend/packages/external-captcha"
npm run build

next_version=$(($(ls $path/../external_generated/versions/ | sort -n | tail -1) + 1))
version_dir=$path/../external_generated/versions/$next_version
mkdir $version_dir

echo "Please add following lines to ya.make:"
echo "# --- $next_version --- "
cd dist
for p in $(ls advanced.*.js checkbox.*.js advanced.*.html checkbox.*.html); do
    cp $p $version_dir/$p
    echo "\${CURDIR}/external_generated/versions/$next_version/$p"
done
cp captcha.js $path/../external_generated/versions/$next_version/$next_version-captcha.js
echo "\${CURDIR}/external_generated/versions/$next_version/$next_version-captcha.js"
echo "# --- /$next_version --- "

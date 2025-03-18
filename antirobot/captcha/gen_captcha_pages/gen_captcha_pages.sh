#!/usr/bin/env bash

set -e

# $1 - path to Node.js

export PATH=$1:$PATH
path=$(pwd)
cd `dirname $0`/../../../frontend/packages/captcha/apps/antirobot

npm ci --registry=https://npm.yandex-team.ru
npm run build


next_version=$(($(ls $path/../smart_generated/versions/ | sort -n | tail -1) + 1))
version_dir=$path/../smart_generated/versions/$next_version
mkdir $version_dir

cp -r build/public/* "$version_dir/"

echo "Please add following lines to ya.make:"
cd $version_dir
rm static_routes.js
rm *.LICENSE.txt
rm -rf static
rm -rf tt
rm -rf uk
rm -rf be
mv en com
mv kk kz

echo "# --- $next_version --- "
for p in $(ls -d */*/index.html); do
    echo "smart_generated/versions/$next_version/$p"
done
echo "# --- /$next_version --- "
echo ""
echo "# --- $next_version --- "
for p in $(ls -d */); do
    lang=$(echo $p | sed 's/\/*$//g')
    if [ "$lang" != "static" ]; then
        echo "\${template_dir_out}/$next_version-captcha_advanced.html.$lang"
        echo "\${template_dir_out}/$next_version-captcha_checkbox.html.$lang"
    fi
done
echo "# --- /$next_version --- "
echo ""
echo "# --- $next_version --- "
for p in $(find . -type f | grep -v ".html"); do
    echo "\${CURDIR}/smart_generated/versions/$next_version/$p"
done
echo "# --- /$next_version --- "

for i in $(ls -d */advanced/index.html); do
    sed -i.bak -e 's/id="advanced-captcha-form"/id="advanced-captcha-form" onsubmit="document.getElementById('\''advanced-captcha-form'\'').action+='\''\&rep='\''+encodeURIComponent(document.querySelector('\''[name=rep]'\'').value)"/' "$i" && rm "$i.bak"
done

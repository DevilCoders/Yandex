#!/usr/bin/env bash

# $1 - path to Node.js

export PATH=$1:$PATH
cd `dirname $0`/../../captcha/source_htmls

npm i
node utils/error-booster-bundle-builder/builder.js
node utils/greed-bundle-builder/builder.js
YENV=production node_modules/.bin/enb make
node utils/hash-static-urls.js

for i in captcha
do
    bundle_dir="desktop.bundles/$i"
    cp $bundle_dir/$i.html ../generated/
    cp $bundle_dir/*.min.css ../generated/
    cp $bundle_dir/*.min.js ../generated/
    cp $bundle_dir/*error-counter.js ../generated/
    cat $bundle_dir/$i.min.js $bundle_dir/captcha.greed.js > ../generated/$i.min.js
done
cp $bundle_dir/js_print_mapping.json ../generated/js_print_mapping.json

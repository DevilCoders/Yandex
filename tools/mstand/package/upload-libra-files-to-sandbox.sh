#!/bin/sh

set -e

TTL=180
UPLOAD_CMD="ya upload --ttl ${TTL} --tar"
# ya package requires single files to be tar'ed (from devtools@: to be able calc checksum for resource)
echo "Uploading blockstat.dict"
${UPLOAD_CMD} --type "BLOCKSTAT_DICT" --description "blockstat.dict for MSTAND package" "blockstat.dict"
echo "Stripping libra.so"
strip "libra.so"
echo "Uploading libra.so"
${UPLOAD_CMD} --type "LIBRA_LIBRARY" --description "libra.so library for MSTAND package" "libra.so"
echo "Uploading browser.xml"
${UPLOAD_CMD} --type "UATRAITS_BROWSER_XML" --description "browser.xml data for MSTAND package" "browser.xml"

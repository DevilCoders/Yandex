#!/bin/bash

>/dev/null
2>/dev/null

CHECK=$(timeout 20 /usr/bin/jhttp.sh -n localhost -p 10000 -o -k -u /ping -w "code: %{http_code};" -e "code: 200;" -f)
ANSWER="0; ok"

if [[ $CHECK != $ANSWER ]]; then
  timeout 20 service cocaine-v12-tornado-proxy start
fi


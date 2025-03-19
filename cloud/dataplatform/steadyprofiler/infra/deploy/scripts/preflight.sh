#!/bin/sh
set -e

ENV=${QLOUD_ENVIRONMENT:-testing}

sed -e "s/%QLOUD_HTTP_PORT%/${QLOUD_HTTP_PORT:-80}/" -i /etc/nginx/sites-enabled/*.conf

supervisorctl start node
supervisorctl start nginx
supervisorctl start server
server {
	listen 80;
	listen 443;

	server_name  team.yandex.st.*;
	access_log /var/log/nginx/team.yandex.st.access.log main;
	root         /usr/local/www/team.yandex.st/;

	gzip on;
	gzip_disable msie6;
	gzip_types text/plain text/css application/x-javascript text/javascript;

	expires +10y;

	rewrite ^(/.*)?/v-[^/]+/(.*)(\?.*)?$ $1/$2$3 break;
}


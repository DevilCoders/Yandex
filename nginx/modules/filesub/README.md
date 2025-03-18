
Filesub
================

Substitute string in response body (and headers) with content of file.
Could be useful if file is huge.

Usage
-----

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;
        location / {
            filesub '</html>' '/etc/nginx/detect.js'
            proxy_pass http://127.0.0.1:8081/;
        }
    }

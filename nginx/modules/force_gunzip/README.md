
Force gunzip
================

Forse gunzip in upstream response body for, e.g. string substitution. 
Response bodies could be compressed for client again.

Incompatible with gunzip module. You have been warned.

Usage
-----

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;
        location / {
            forse_gunzip on;
            gzip_vary on;
            proxy_pass http://127.0.0.1:8081/;
            proxy_set_header Accept-Encoding gzip;
        }
    }

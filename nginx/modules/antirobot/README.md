nginx-yandex-antirobot-module
=============================

nginx module for communication with [Yandex Antirobot service](https://wiki.yandex-team.ru/jandekspoisk/sepe/antirobotonline/docs/service)

Bonus: all the Nginx upstream features (including keepalive) can be used for communication with antirobot engine

**Warning!** Not compatible with nginx SPDY and HTTP2 modules!

**Please, try not to use it - use [AWACS easy mode](https://wiki.yandex-team.ru/awacs/top-level-easy-mode/#antirobot) instead!**


Directives
==========

antirobot_request
-----------------
**syntax:** *antirobot_request /antirobot-(no|with)-captcha

**default:** *disabled*

**context:** *http, server, location*

Sets encryption key.

Possible values:

    /location - set antirobot handlers location
    off - disable

antirobot_request_set
---------------------

See http://nginx.org/ru/docs/http/ngx_http_auth_request_module.html for details.




Variables
=========

$antirobot_status
-----------------

Can be set to one of specified values:

    bypass - antirobot answered with code 200 and control header was not set

    captcha - antirobot answered with code 200 or 302 and control header was set

    error - antirobot response has invalid code (not 200/302/403) or timed out, request bypassed

    denied - antirobot answered with code 403 and control header was set


Installation from arcadia
============

```bash
cd ~/arcadia/nginx/bin
ya make . -r
```
or
```bash
cd ~/arcadia/nginx/bin-noperl
ya make . -r -DOS_SDK=ubuntu-16
```

Example configuration
=====================

    upstream antirobot_upstream {
        # please discuss it with antirobot team (create task https://st.yandex-team.ru/createTicket?queue=CAPTCHA&_form=89632)
        server antirobot.yandex.ru:80 max_fails=0;
        keepalive 2; # 2 keepalive connections to antirobot
    }

    server {
        listen       80;
        server_name  something.yandex.ru;

        location / {
            antirobot_request /antirobot-with-captcha;
            proxy_pass http://frontend_application:8181;
        }

        location /static/ {
            antirobot_request /antirobot-no-captcha;
            root /var/www/html;
        }

        # this location does not show captcha
        location = /antirobot-no-captcha {
          internal;
          antirobot_request off; # fool-protection
          proxy_connect_timeout   200ms;
          proxy_send_timeout      200ms;
          proxy_read_timeout      200ms;
      #    proxy_buffer_size 128k; # please, set appropriate value to fit antirobot response body (if required)
      #    proxy_buffers 9 64k;
      #    proxy_buffering on;
      #    proxy_set_header X-Antirobot-Service-Y "market"; # please, discuss it with antirobot team
          proxy_set_header X-Antirobot-MayBanFor-Y 0;
          proxy_set_header X-Host-Y $host;
          proxy_set_header X-Forwarded-For-Y $remote_addr;
          proxy_set_header X-TLS-Cipher-Y $ssl_cipher;
          proxy_set_header X-Yandex-HTTPS "yes";
          proxy_pass http://antirobot_upstream$request_uri;          
        }

        # this location shows captcha
        location = /antirobot-with-captcha {
            internal;
            antirobot_request off; # fool-protection
            proxy_connect_timeout   200ms;
            proxy_send_timeout      200ms;
            proxy_read_timeout      200ms;
        #    proxy_buffer_size 128k; # please, set appropriate value to fit antirobot response body (if required)
        #    proxy_buffers 9 64k;
        #    proxy_buffering on;
        #    proxy_set_header X-Antirobot-Service-Y "market"; # please, discuss it with antirobot team
            proxy_set_header X-Antirobot-MayBanFor-Y 1;
            proxy_set_header X-Host-Y $host;
            proxy_set_header X-Forwarded-For-Y $remote_addr;
            proxy_set_header X-TLS-Cipher-Y $ssl_cipher;
            proxy_set_header X-Yandex-HTTPS "yes";
            proxy_pass http://antirobot_upstream$request_uri;
        }

        location ~* ^/(captcha|showcaptcha|x?checkcaptcha|tmgrdfrend).*$ {
            proxy_connect_timeout   1000ms;
            proxy_send_timeout      1000ms;
            proxy_read_timeout      1000ms;
            proxy_hide_header X-ForwardToUser-Y;
        #    proxy_set_header X-Antirobot-Service-Y "market"; # please, discuss it with antirobot team
            proxy_set_header X-Host-Y $host;
            proxy_set_header X-Forwarded-For-Y $remote_addr;
            proxy_set_header X-TLS-Cipher-Y $ssl_cipher;
            proxy_set_header X-Yandex-HTTPS "yes";
            proxy_pass http://antirobot_upstream;
        }
    }


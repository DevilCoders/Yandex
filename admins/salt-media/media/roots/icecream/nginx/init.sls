/etc/nginx/sites-enabled/icecream.conf:
  file.managed:
    - source: salt://icecream/nginx/icecream.conf
/etc/nginx/sites-enabled/default:
  file.absent

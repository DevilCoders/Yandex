#!/bin/bash
echo > debian/links
for i in `ls nginx/sites-available`; do echo -e "etc/nginx/sites-available/$i etc/nginx/sites-enabled/$i" >> debian/links; done

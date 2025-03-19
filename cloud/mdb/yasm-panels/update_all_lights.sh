#!/bin/sh

executer --cached 'exec dashing01h.mail.yandex.net,dashing01f.mail.yandex.net mkdir -p /var/www/yasm'

./gen_lights.py 30 mail_xdb 01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,301,302,303,304,305,306,307,308,309,310,311,312,313,314,315,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015,2016,2017,2018,2019,2020,1000 templates/xdb_lights.json templates/lights.html > out.html

executer --cached 'distribute dashing01h.mail.yandex.net,dashing01f.mail.yandex.net out.html /var/www/yasm/mail_xdb.html'

./gen_lights.py 18 mail_xdb 01,02,03,04,05,06,07,08,09,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,301,302,303,304,305,306,307,308,309,310,311,312,313,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015,2016,2017,2018,2019,2020 templates/xdb_lights_small.json templates/lights.html > out.html

executer --cached 'distribute dashing01h.mail.yandex.net,dashing01f.mail.yandex.net out.html /var/www/yasm/mail_xdb_light.html'

./gen_lights.py 13 mail_otherdb mail-rpopdb:rpopdb,mail-xivadb:xivadb,mail-xivastore:xivadb,mail-mopsdb:mopsdb,mail-setdb:setdb,mail-corpdb,mail-sharpei:sharddb,mail-s3meta:s3meta,mail-s3db:s3db templates/lights.json templates/lights.html > out.html

executer --cached 'distribute dashing01h.mail.yandex.net,dashing01f.mail.yandex.net out.html /var/www/yasm/mail_otherdb.html'

./gen_lights.py 13 maildb mail-rpopdb:rpopdb,mail-xivadb:xivadb,mail-xivastore:xivadb,mail-mopsdb:mopsdb,mail-setdb:setdb,mail-corpdb,mail-s3meta:s3meta,mail-s3db:s3db,mail-xdb-hot:maildb,mail-xdb-warm:maildb,mail-xdb-cold:maildb,mail-xdb-corp:maildb templates/lights.json templates/lights.html > out.html

executer --cached 'distribute dashing01h.mail.yandex.net,dashing01f.mail.yandex.net out.html /var/www/yasm/maildb.html'

./gen_lights.py 9 mail_pgproxy rpopdb,xivadb,xivastore,setdb,s3db,xivadb_corp,setdb_corp templates/pgproxy_lights.json templates/lights.html > out.html

executer --cached 'distribute dashing01h.mail.yandex.net,dashing01f.mail.yandex.net out.html /var/www/yasm/mail_pgproxy.html'

./gen_lights.py 9 mail_pgproxy rpopdb,xivadb,xivastore,setdb,s3db,xivadb_corp,setdb_corp templates/pgproxy_lights_with_tps.json templates/lights.html > out.html

executer --cached 'distribute dashing01h.mail.yandex.net,dashing01f.mail.yandex.net out.html /var/www/yasm/mail_pgproxy_tps.html'

rm -f out.html

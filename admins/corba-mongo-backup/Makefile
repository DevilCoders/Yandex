install:
	install -m755 -D src/mongo-backup.sh $(DESTDIR)/usr/bin/mongo-backup.sh
	install -m755 -D src/mongo-oplog-backup.sh $(DESTDIR)/usr/bin/mongo-oplog-backup.sh
	install -m755 -D src/check-mongo-backup.sh $(DESTDIR)/usr/bin/check-mongo-backup.sh
	install -m644 -D src/mongo-backup.conf $(DESTDIR)/etc/mongo-backup-default.conf
	install -m644 -D src/distributed-flock-mongo-backup.json $(DESTDIR)/etc/distributed-flock-mongo-backup.json
	install -m644 -D src/zkflock.logrotate $(DESTDIR)/etc/logrotate.d/zkflock
	install -m644 -D src/mongo-backup.cron $(DESTDIR)/etc/cron.d/mongo-backup
	grep -i exit src/mongo-backup.sh | egrep -o "\-?[0-9]+" | sort -un > src/known-codes
	install -m644 -D src/known-codes $(DESTDIR)/var/lib/mongo-backup/known-codes
	install -m644 -D src/check-mongo-backup.conf $(DESTDIR)/etc/monrun/conf.d/check-mongo-backup.conf

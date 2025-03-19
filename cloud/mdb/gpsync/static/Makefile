install:
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/etc/sudoers.d
	install -d $(DESTDIR)/etc/logrotate.d
	install -d $(DESTDIR)/opt/greenplum-db-6/bin
	install -m400 gpsync.sudoers.d $(DESTDIR)/etc/sudoers.d/gpsync
	install -m644 gpsync.logrotate $(DESTDIR)/etc/logrotate.d/gpsync
	install -m644 gpsync.service $(DESTDIR)/lib/systemd/system/gpsync.service
	install -m755 gpstartdisp.py $(DESTDIR)/opt/greenplum-db-6/bin/gpstartdisp
	install -m755 gp_lsclusters $(DESTDIR)/opt/greenplum-db-6/bin/gp_lsclusters

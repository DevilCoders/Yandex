build:
	ya make

test:
	ya make -tt --no-test-outputs --no-src-links

fix:
	ya tool yo fix .

ydb:
	bash -e dev/ydb/run_ydb.sh;

deploy::
	pssh run -p 6 -a "sudo sed -i 's/{% set license_server_tag=.*/{% set license_server_tag=\"$(VERSION)\" %}/g' /srv/yc/ci/salt-formulae/marketplace/data/srv/salt/pillar/roles/marketplace.sls" C@cloud_preprod_marketplace
	pssh run -p 6 -a 'sudo salt-call --local --config-dir /srv/yc/ci/salt-formulae/marketplace/data/etc/salt state.sls roles.marketplace queue=True' C@cloud_preprod_marketplace

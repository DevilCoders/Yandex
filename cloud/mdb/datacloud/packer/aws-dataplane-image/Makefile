.PHONY=clean
clean:
	rm conf/mdb-bionic.gpg || true
	rm conf/master-sign.pub || true
	rm conf/allCAs.pem || true

conf/mdb-bionic.gpg:
	@echo "Downloading gpg key"
	mkdir -p conf && ya vault get version ver-01f3at3mfd1q9da64dz52pqeg8 -o public_key > conf/mdb-bionic.gpg

conf/master-sign.pub:
	@echo "Downloading salt master sign public key"
	mkdir -p conf && ya vault get version ver-01f7k7mhy5ty5yvnsz5rd8646x -o master-sign > conf/master-sign.pub

conf/allCAs.pem:
	@echo "Downloading allCAs cert"
	mkdir -p conf && ya vault get version ver-01f7k7mhy5ty5yvnsz5rd8646x -o all-cas > conf/allCAs.pem

.PHONY=keys
keys: conf/mdb-bionic.gpg conf/master-sign.pub conf/allCAs.pem

.PHONY=validate
validate: keys
	@echo "Start validation"
	packer validate ./dc-aws-image.pkr.hcl

.PHONY=fix
fix: keys
	packer fix ./dc-aws-image.pkr.hcl

.PHONY=clickhouse-image
clickhouse-image: validate keys
	@echo "Start building clickhouse image"
	packer build -var 'image_type=ch' ./dc-aws-image.pkr.hcl

.PHONY=zookeeper-image
zookeeper-image: validate keys
	@echo "Start building zookeeper image"
	packer build -var 'image_type=zk' ./dc-aws-image.pkr.hcl

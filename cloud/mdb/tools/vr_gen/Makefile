.PHONY: vr_gen

OUT_DIR = ../../salt/salt/components/pg-dbs/dbaas_metadb/conf/valid_resources/
ENVS = compute_preprod compute_prod porto_prod porto_test

vr_gen:
	ya make

${ENVS}: vr_gen
		./vr_gen ../../salt/pillar/$(subst _,/,$@)/metadb/ ${OUT_DIR} $@

israel_prod: vr_gen
	./vr_gen ../../salt/pillar/mdb_controlplane_israel/metadb/resources/ ${OUT_DIR} $@

generate_valid_resources: ${ENVS}

help:
	@echo 'Common targets:'
	@echo '  generate_valid_resources  Generate all valid_resources'
	@echo '  <env>                     Generate specific environment valid_resources'
	@echo '  help                      Show this help message.'

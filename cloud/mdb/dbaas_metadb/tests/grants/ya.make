PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb)

DATA(
    arcadia/cloud/mdb/dbaas_metadb/head/grants
    arcadia/cloud/mdb/salt/pillar/mdb_controlplane_porto_test/mdb_meta_test.sls
    arcadia/cloud/mdb/salt/pillar/mdb_controlplane_porto_prod/mdb_meta_prod.sls
    arcadia/cloud/mdb/salt/pillar/mdb_controlplane_compute_preprod/mdb_meta_preprod.sls
    arcadia/cloud/mdb/salt/pillar/mdb_controlplane_compute_prod/mdb_meta_prod.sls
)

PEERDIR(
    cloud/mdb/dbaas_metadb/tests/grants/lib
)

TEST_SRCS(test_grants.py)

END()

RECURSE(
    lib
    gen
)

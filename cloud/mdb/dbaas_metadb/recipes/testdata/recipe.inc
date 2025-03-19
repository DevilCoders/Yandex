USE_RECIPE(cloud/mdb/dbaas_metadb/recipes/testdata/metadb_testdata_recipe)
DEPENDS(cloud/mdb/dbaas_metadb/recipes/testdata)
DEPENDS(cloud/mdb/dbaas_metadb/bin)

DATA(arcadia/cloud/mdb/dbaas-internal-api-image)
DATA(arcadia/cloud/mdb/mdb-internal-api/functest)
DATA(arcadia/cloud/mdb/salt/pillar/metadb_default_versions.sls)
DATA(arcadia/cloud/mdb/salt/pillar/metadb_default_alert.sls)

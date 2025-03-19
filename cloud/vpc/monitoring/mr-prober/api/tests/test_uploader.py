import database
import settings
from agent.config import models
from api.agents import get_prober_configs_with_null_cluster_id
from api.client import ModelBasedHttpClient
from api.tests.common import (
    create_test_prober,
    create_test_prober_config,
    create_test_recipe_and_cluster,

    testdata_create_prober_config_request,
)


def test_get_prober_configs_with_null_cluster_id(test_database, client: ModelBasedHttpClient, mocked_s3):
    db = database.session_maker()
    bucket = mocked_s3.Bucket(settings.AGENT_CONFIGURATIONS_S3_BUCKET)

    # 1. Check that the database is empty
    assert len(get_prober_configs_with_null_cluster_id(db)) == 0, "not empty database"

    # 2. Create the cluster, recipe, prober and prober config with the cluster_id
    prober = create_test_prober(client)
    _, cluster = create_test_recipe_and_cluster(client)
    _ = create_test_prober_config(
        client, prober, create_request=testdata_create_prober_config_request.copy(update={"cluster_id": cluster.id})
    )

    # 3. Check that the method returns an empty list
    assert len(get_prober_configs_with_null_cluster_id(db)) == 0, "prober with cluster_id must be omitted"

    # 4. Check that the created cluster has 1 prober configs
    obj_data = bucket.Object(f"{settings.S3_PREFIX}clusters/{cluster.id}/cluster.json").get()
    cluster_raw_data = obj_data["Body"].read().decode()
    cluster_config = models.Cluster.parse_raw(cluster_raw_data)
    assert len(cluster_config.prober_configs) == 1

    # 5. Create prober and prober config without cluster_id
    second_prober = create_test_prober(client)
    second_config = create_test_prober_config(client, second_prober)

    # 6. Check that the method returns list with one element
    prober_configs_list = get_prober_configs_with_null_cluster_id(db)
    assert len(prober_configs_list) == 1
    assert prober_configs_list[0].id == second_config.id

    # 7. Check that the created cluster has 2 prober configs
    obj_data = bucket.Object(f"{settings.S3_PREFIX}clusters/{cluster.id}/cluster.json").get()
    cluster_raw_data = obj_data["Body"].read().decode()
    cluster_config = models.Cluster.parse_raw(cluster_raw_data)
    assert len(cluster_config.prober_configs) == 2

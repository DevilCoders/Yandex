from sqlalchemy.orm import Session
from starlette import status

import database
import database.models as db_models
from api import models
from api.client import ModelBasedHttpClient
from api.tests.common import (
    create_test_prober,
    create_test_prober_config,
    create_test_recipe_and_cluster,
    copy_configs_to_prober,
    move_configs_to_prober,
    delete_prober_config,
    testdata_create_prober_config_request,
    testdata_create_prober_config_with_variables_request,
)


def test_create_prober_config(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    _config = create_test_prober_config(client, prober)

    new_prober = client.get(f"/probers/{prober.id}/", response_model=models.ProberResponse).prober
    assert len(new_prober.configs) == 1

    assert new_prober.configs[0].manually_created
    assert new_prober.configs[0].is_prober_enabled
    assert new_prober.configs[0].default_routing_interface == "eth0"

    assert new_prober.configs[0] == _config


def test_update_prober_config(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create prober and config for it
    prober = create_test_prober(client)
    config = create_test_prober_config(client, prober)

    # 2. Update the prober config
    response = client.put(
        f"/probers/{prober.id}/configs/{config.id}",
        json=models.UpdateProberConfigRequest(
            is_prober_enabled=True,
            s3_logs_policy="FAIL",
            variables={
                "variable-key-1": "variable-value-1"
            },
            matrix_variables={
                "matrix-variable-key-1": ["matrix-variable-value-1", "matrix-variable-value-2"]
            }
        ),
        response_model=models.UpdateProberConfigResponse,
    )

    # 3. Check that config is updated
    assert len(response.config.variables) == 1
    assert len(response.config.matrix_variables) == 1

    # 4. Check that prober is updated too
    new_prober = client.get(f"/probers/{prober.id}/", response_model=models.ProberResponse).prober
    assert len(new_prober.configs) == 1

    assert new_prober.configs[0].manually_created
    assert new_prober.configs[0].is_prober_enabled
    assert new_prober.configs[0].s3_logs_policy == "FAIL"
    assert new_prober.configs[0].variables == [
        models.ProberVariable(id=1, name="variable-key-1", value="variable-value-1")
    ]
    assert new_prober.configs[0].matrix_variables == [
        models.ProberMatrixVariable(
            id=1, name="matrix-variable-key-1", values=["matrix-variable-value-1", "matrix-variable-value-2"]
        )
    ]


def test_update_prober_config_delete_variable(client: ModelBasedHttpClient, mocked_s3, db: Session):
    # 1. Create prober and config for it
    prober = create_test_prober(client)
    config = create_test_prober_config(client, prober, models.CreateProberConfigRequest(
        is_prober_enabled=True,
        variables={
            "variable-key-1": "variable-value-1"
        },
        matrix_variables={
            "matrix-variable-key-1": ["matrix-variable-value-1", "matrix-variable-value-2"]
        }
    ))

    # 2. Update the prober config: delete variable and matrix variable
    response = client.put(
        f"/probers/{prober.id}/configs/{config.id}",
        json=models.UpdateProberConfigRequest(
            is_prober_enabled=True,
            s3_logs_policy="FAIL",
        ),
        response_model=models.UpdateProberConfigResponse,
    )

    # 3. Check that config is updated
    assert len(response.config.variables) == 0
    assert len(response.config.matrix_variables) == 0

    # 4. Check that prober is updated too
    new_prober = client.get(f"/probers/{prober.id}/", response_model=models.ProberResponse).prober
    assert len(new_prober.configs) == 1

    assert new_prober.configs[0].manually_created
    assert new_prober.configs[0].is_prober_enabled
    assert new_prober.configs[0].s3_logs_policy == "FAIL"
    assert new_prober.configs[0].variables == []
    assert new_prober.configs[0].matrix_variables == []

    # 5. Check that variables have been removed from the database
    assert db.query(database.models.ProberVariable).count() == 0
    assert db.query(database.models.ProberMatrixVariable).count() == 0


def test_copy_prober_configs(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    config = create_test_prober_config(client, prober)
    second_prober = create_test_prober(client)
    check_copy_config_from_one_prober_to_other(client, prober, config, second_prober)


def test_copy_prober_configs_with_cluster_id(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    _, cluster = create_test_recipe_and_cluster(client)
    config = create_test_prober_config(
        client, prober, create_request=testdata_create_prober_config_request.copy(update={"cluster_id": cluster.id})
    )
    second_prober = create_test_prober(client)
    check_copy_config_from_one_prober_to_other(client, prober, config, second_prober)


def check_copy_config_from_one_prober_to_other(
    client: ModelBasedHttpClient, prober: models.Prober, config: models.ProberConfig, second_prober: models.Prober
):
    # 1. Check that second prober has no configs
    response = client.get(
        f"/probers/{second_prober.id}",
        response_model=models.ProberResponse
    )
    assert len(response.prober.configs) == 0

    # 2. Copy configs from first prober to second one
    second_prober = copy_configs_to_prober(client, prober.id, second_prober.id)

    # 3. Check that second prober has configs now
    assert len(second_prober.configs) == 1
    assert second_prober.configs[0].id != config.id
    assert second_prober.configs[0].is_prober_enabled == config.is_prober_enabled
    second_prober_config_cluster_id = second_prober.configs[0].cluster_id

    # 4. Check that first prober has it's config too
    response = client.get(
        f"/probers/{prober.id}",
        response_model=models.ProberResponse,
    )
    assert len(response.prober.configs) == 1
    assert response.prober.configs[0] == config
    assert second_prober_config_cluster_id == response.prober.configs[0].cluster_id


def test_copy_prober_configs_prohibited_for_self_copying(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    create_test_prober_config(client, prober)

    client.post(
        f"/probers/{prober.id}/configs/copy",
        json=models.CopyOrMoveProberConfigsRequest(
            prober_id=prober.id,
        ),
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
    )


def test_move_prober_configs(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    config = create_test_prober_config(client, prober)
    second_prober = create_test_prober(client)

    # 1. Check that second prober has no configs
    response = client.get(
        f"/probers/{second_prober.id}",
        response_model=models.ProberResponse
    )
    assert len(response.prober.configs) == 0

    # 2. Move configs from first prober to second one
    second_prober = move_configs_to_prober(client, prober.id, second_prober.id)

    # 3. Check that second prober has configs now
    assert len(second_prober.configs) == 1
    assert second_prober.configs[0] == config

    # 4. Check that first prober has no configs now, because they have been moved to second one
    response = client.get(
        f"/probers/{prober.id}",
        response_model=models.ProberResponse,
    )
    assert len(response.prober.configs) == 0


def test_prober_config_variables(client: ModelBasedHttpClient, mocked_s3, test_database):
    db = database.session_maker()
    prober = create_test_prober(client)
    prober_config = create_test_prober_config(
        client, prober, create_request=testdata_create_prober_config_with_variables_request
    )
    second_prober = create_test_prober(client)
    test_variables = testdata_create_prober_config_with_variables_request.variables
    # tuple will be converted to list
    test_variables["tuple_var"] = list(test_variables["tuple_var"])

    # 0. Check that prober_config's variables are equal to testdata's
    assert {var.name: var.value for var in prober_config.variables} == test_variables

    # 1. Copy configs from the first prober to the second one
    second_prober = copy_configs_to_prober(client, prober.id, second_prober.id)
    assert len(second_prober.configs) == 1

    # 2. Check that second prober has right config variables
    second_prober_config = second_prober.configs[0]
    assert {var.name: var.value for var in second_prober_config.variables} == test_variables

    # 3. Delete config of the second prober
    second_prober = delete_prober_config(client, second_prober.id, second_prober_config.id)

    # 4. Check that all variables that belonged to the second prober are deleted
    assert db.query(db_models.ProberVariable).filter(db_models.ProberVariable.prober_config_id.is_(None)).count() == 0

    # 5. Move configs from the first prober to the second one
    second_prober = move_configs_to_prober(client, prober.id, second_prober.id)
    assert len(second_prober.configs) == 1

    # 6. Check that second prober has right config variables
    second_prober_config = second_prober.configs[0]
    assert {var.name: var.value for var in second_prober_config.variables} == test_variables

    # 7. Check that all variables that belonged to the first prober now are belongs to second prober
    assert db.query(db_models.ProberVariable).count() == len(test_variables)


def test_prober_config_matrix_variables(client: ModelBasedHttpClient, mocked_s3):
    prober = create_test_prober(client)
    prober_config = create_test_prober_config(
        client, prober, create_request=testdata_create_prober_config_with_variables_request
    )
    second_prober = create_test_prober(client)

    test_matrix_variables = testdata_create_prober_config_with_variables_request.matrix_variables

    # 0. Check that prober_config's matrix_variables are equal to testdata's
    assert {var.name: var.values for var in prober_config.matrix_variables} == test_matrix_variables

    # 1. Copy configs from the first prober to the second one
    second_prober = copy_configs_to_prober(client, prober.id, second_prober.id)

    # 2. Check that second prober has correct config matrix_variables
    second_prober_config = second_prober.configs[0]
    assert {var.name: var.values for var in second_prober_config.matrix_variables} == test_matrix_variables

    # 3. Delete config of the second prober
    second_prober = delete_prober_config(client, second_prober.id, second_prober_config.id)

    # 5. Move configs from the first prober to the second one
    second_prober = move_configs_to_prober(client, prober.id, second_prober.id)

    # 6. Check that second prober has right config variables
    second_prober_config = second_prober.configs[0]
    assert {var.name: var.values for var in second_prober_config.matrix_variables} == test_matrix_variables


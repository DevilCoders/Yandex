from datetime import datetime, timedelta

import pytz

import common.tests.conftest
import database
from database.models import Cluster, ClusterDeployPolicyType, ManualClusterDeployPolicy, ClusterRecipe, \
    RegularClusterDeployPolicy

test_database = common.tests.conftest.test_database


def _create_recipe(db) -> ClusterRecipe:
    recipe = ClusterRecipe(
        manually_created=True,
        arcadia_path="",
        name="Test Recipe",
        description="",
    )

    db.add(recipe)
    db.commit()

    return recipe


def test_manual_dp_is_ready_to_deploy(test_database):
    db = database.session_maker()
    recipe = _create_recipe(db)

    deploy_policy = ManualClusterDeployPolicy(
        parallelism=10,
        type=ClusterDeployPolicyType.MANUAL,
        ship=True,
        plan_timeout=10,
        apply_timeout=10,
    )

    cluster = Cluster(
        recipe_id=recipe.id,
        name="Test",
        slug="test",
        manually_created=True,
        arcadia_path="",
        variables=[],
        deploy_policy=deploy_policy,
    )

    db.add(cluster)
    db.commit()

    assert cluster.is_ready_for_deploy()

    db.close()


def test_manual_dp_is_not_ready_to_deploy(test_database):
    db = database.session_maker()
    recipe = _create_recipe(db)

    deploy_policy = ManualClusterDeployPolicy(
        parallelism=10,
        type=ClusterDeployPolicyType.MANUAL,
        ship=False,
        plan_timeout=10,
        apply_timeout=10,
    )
    cluster = Cluster(
        recipe_id=recipe.id,
        name="Test",
        slug="test",
        manually_created=True,
        arcadia_path="",
        variables=[],
        deploy_policy=deploy_policy,
    )

    db.add(cluster)
    db.commit()

    assert not cluster.is_ready_for_deploy()

    db.close()


def test_regular_dp_is_ready_to_deploy(test_database):
    db = database.session_maker()
    recipe = _create_recipe(db)

    dt = datetime.now(pytz.utc)
    last_attempt = dt - timedelta(days=4)

    deploy_policy = RegularClusterDeployPolicy(
        parallelism=10,
        type=ClusterDeployPolicyType.REGULAR,
        sleep_interval=600,
        plan_timeout=10,
        apply_timeout=10,
    )
    cluster = Cluster(
        recipe_id=recipe.id,
        name="Test",
        slug="test",
        manually_created=True,
        arcadia_path="",
        variables=[],
        deploy_policy=deploy_policy,
        last_deploy_attempt_finish_time=last_attempt
    )

    db.add(cluster)
    db.commit()

    assert cluster.is_ready_for_deploy()

    db.close()


def test_regular_dp_is_not_ready_to_deploy(test_database):
    db = database.session_maker()
    recipe = _create_recipe(db)

    deploy_policy = RegularClusterDeployPolicy(
        parallelism=10,
        type=ClusterDeployPolicyType.REGULAR,
        sleep_interval=600,
        plan_timeout=10,
        apply_timeout=10,
    )
    cluster = Cluster(
        recipe_id=recipe.id,
        name="Test",
        slug="test",
        manually_created=True,
        arcadia_path="",
        variables=[],
        deploy_policy=deploy_policy,
        last_deploy_attempt_finish_time=datetime.now(pytz.utc) - timedelta(seconds=4)
    )

    db.add(cluster)
    db.commit()

    assert not cluster.is_ready_for_deploy()

    db.close()

from pathlib import Path

import api
from api.models import BashProberRunner
from database.models import ClusterDeployPolicyType
from tools.synchronize.data import models as data_models
from tools.synchronize.data.collection import ObjectsCollection
from tools.synchronize.diff import DiffBuilder

test_recipe = data_models.Recipe(
    id=None,
    arcadia_path="recipes/meeseeks/recipe.yaml",
    name="IaC Recipe",
    description="IaC recipe",
    files=[],
)

test_deploy_policy = api.models.ManualClusterDeployPolicy(
    parallelism=20,
    ship=False,
    plan_timeout=360,
    apply_timeout=3600,
)

test_cluster = data_models.Cluster(
    id=None,
    arcadia_path="clusters/prod/meeseeks/cluster.yaml",
    name="IaC cluster",
    slug="iac-cluster",
    recipe=test_recipe,
    variables={"key": "values"},
    deploy_policy=test_deploy_policy,
)

test_prober_file = data_models.ProberFile(
    relative_file_path="dns.sh",
    is_executable=True,
    content=b"#!/bin/bash"
)

test_prober_config = data_models.ProberConfig(interval_seconds=60, timeout_seconds=10, hosts_re="agent-.*")

test_prober = data_models.Prober(
    id=None,
    arcadia_path="probers/network/dns/prober.yaml",
    name="DNS",
    slug="dns",
    description="Simple DNS check",
    runner=BashProberRunner(
        command="./dns.sh",
    ),
    files=[test_prober_file],
    configs=[test_prober_config],
)

test_cluster_with_untracked_variables_loaded_from_iac = data_models.Cluster(
    id=None,
    arcadia_path="clusters/some-environment/iac_untracked_variable/cluster.yaml",
    name="IaC Untracked var",
    slug="untracked_var",
    recipe=test_recipe,
    variables={"key": "values", "compute_nodes": []},
    untracked_variables=["compute_nodes"],
)

test_cluster_with_untracked_variables_loaded_from_api = data_models.Cluster(
    id=11,
    arcadia_path="clusters/some-environment/iac_untracked_variable/cluster.yaml",
    name="IaC Untracked var",
    slug="untracked_var",
    recipe=test_recipe,
    variables={"key": "values", "compute_nodes": ["test-node1", "test-node2"]},
    untracked_variables=[],
)


def test_diff_builder_create_recipe_and_cluster():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster,
        },
        recipes={
            test_recipe.arcadia_path: test_recipe,
        },
    )
    api_collection = ObjectsCollection()

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.recipes.added == [test_recipe]
    assert diff.recipes.changed == []
    assert diff.recipes.deleted == []

    assert diff.clusters.added == [test_cluster]
    assert diff.clusters.changed == []
    assert diff.clusters.deleted == []

    assert diff.probers.is_empty()


def test_diff_builder_include_path_filter():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster,
        },
        recipes={
            test_recipe.arcadia_path: test_recipe,
        },
    )
    api_collection = ObjectsCollection(
        recipes={
            test_recipe.arcadia_path + ".1": test_recipe,
        }
    )

    # DiffBuilder only compares parents in include_path, not exact file locations
    test_cluster_parent = Path(test_cluster.arcadia_path).parents[0]
    diff = DiffBuilder(include=[test_cluster_parent]).build(iac_collection, api_collection)

    # Even though one recipe should be created and one is added, they should be filtered by include_path
    assert diff.recipes.added == []
    assert diff.recipes.changed == []
    assert diff.recipes.deleted == []

    assert diff.clusters.added == [test_cluster]
    assert diff.clusters.changed == []
    assert diff.clusters.deleted == []

    assert diff.probers.is_empty()


def test_diff_builder_empty_if_nothing_changed():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )
    api_recipe = test_recipe.copy(update={"id": 100})
    api_cluster = test_cluster.copy(update={"id": 500, "recipe": api_recipe})
    api_collection = ObjectsCollection(
        clusters={
            api_cluster.arcadia_path: api_cluster,
        },
        recipes={
            api_recipe.arcadia_path: api_recipe,
        },
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.is_empty()


def test_diff_builder_if_cluster_changed():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )
    api_recipe = test_recipe.copy(update={"id": 100})
    api_cluster = test_cluster.copy(update={"id": 500, "recipe": api_recipe, "name": "new-name!"})
    api_collection = ObjectsCollection(
        clusters={
            api_cluster.arcadia_path: api_cluster,
        },
        recipes={
            api_recipe.arcadia_path: api_recipe,
        },
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.recipes.is_empty()
    assert diff.probers.is_empty()

    assert diff.clusters.added == []
    assert diff.clusters.changed == [(api_cluster, test_cluster)]
    assert diff.clusters.deleted == []


def test_diff_builder_if_cluster_deploy_policy_changed():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster.arcadia_path: test_cluster
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )
    api_recipe = test_recipe.copy(update={"id": 100})
    api_deploy_policy = test_deploy_policy.copy(
        update={"id": 100, "type": ClusterDeployPolicyType.MANUAL, "ship": True}
    )
    api_cluster = test_cluster.copy(update={"id": 500, "recipe": api_recipe, "deploy_policy": api_deploy_policy})
    api_collection = ObjectsCollection(
        clusters={
            api_cluster.arcadia_path: api_cluster,
        },
        recipes={
            api_recipe.arcadia_path: api_recipe,
        },
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.recipes.is_empty()
    assert diff.probers.is_empty()

    assert diff.clusters.added == []
    assert diff.clusters.changed == [(api_cluster, test_cluster)]
    assert diff.clusters.deleted == []


def test_diff_builder_if_untracked_vars_present():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster_with_untracked_variables_loaded_from_iac.arcadia_path: test_cluster_with_untracked_variables_loaded_from_iac
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )

    api_recipe = test_recipe.copy(update={"id": 100})
    api_collection = ObjectsCollection(
        clusters={
            test_cluster_with_untracked_variables_loaded_from_api.arcadia_path: test_cluster_with_untracked_variables_loaded_from_api.copy(
                update={"recipe": api_recipe}
            ),
        },
        recipes={
            api_recipe.arcadia_path: api_recipe,
        },
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.is_empty()


def test_diff_builder_if_cluster_changed_and_untracked_vars_present():
    iac_collection = ObjectsCollection(
        clusters={
            test_cluster_with_untracked_variables_loaded_from_iac.arcadia_path: test_cluster_with_untracked_variables_loaded_from_iac
        },
        recipes={
            test_recipe.arcadia_path: test_recipe
        },
    )

    api_recipe = test_recipe.copy(update={"id": 100})
    api_cluster = test_cluster_with_untracked_variables_loaded_from_api.copy(
        update={"id": 500, "recipe": api_recipe, "name": "new-name!"}
    )
    api_collection = ObjectsCollection(
        clusters={
            api_cluster.arcadia_path: api_cluster,
        },
        recipes={
            api_recipe.arcadia_path: api_recipe,
        },
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.recipes.is_empty()
    assert diff.probers.is_empty()

    assert diff.clusters.added == []
    assert len(diff.clusters.changed) == 1
    assert diff.clusters.changed[0][0].variables == diff.clusters.changed[0][1].variables
    assert diff.clusters.deleted == []


def test_diff_builder_if_prober_changed():
    iac_collection = ObjectsCollection(
        probers={
            test_prober.arcadia_path: test_prober
        }
    )
    # Change file's content, ...
    api_prober_file = test_prober_file.copy(update={"id": 100, "content": "#!/usr/bin/python"})
    api_prober = test_prober.copy(update={"id": 200, "files": [api_prober_file]})
    api_collection = ObjectsCollection(
        probers={
            api_prober.arcadia_path: api_prober,
        }
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.recipes.is_empty()
    assert diff.clusters.is_empty()

    assert diff.probers.added == []
    # ... and check that prober is marked as changed in diff
    assert diff.probers.changed == [(api_prober, test_prober)]
    assert diff.probers.deleted == []


def test_diff_builder_produces_empty_diff_if_prober_files_not_changed():
    iac_collection = ObjectsCollection(
        probers={
            test_prober.arcadia_path: test_prober
        }
    )
    api_prober_file = test_prober_file.copy(update={"id": 100})
    api_prober = test_prober.copy(update={"id": 200, "files": [api_prober_file]})
    api_collection = ObjectsCollection(
        probers={
            api_prober.arcadia_path: api_prober,
        }
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    assert diff.probers.is_empty()


def test_diff_builder_cluster_not_marked_as_changed_if_recipe_changed():
    # 1. Create IaC recipe differs from API one by name
    changed_test_recipe = test_recipe.copy(update={"name": "New name"})
    # 2. IaC cluster has been created from this recipe
    changed_test_cluster = test_cluster.copy(update={"recipe": changed_test_recipe})
    iac_collection = ObjectsCollection(
        recipes={test_recipe.arcadia_path: changed_test_recipe},
        clusters={test_cluster.arcadia_path: changed_test_cluster},
    )

    # 3. API collection is the same, one cluster created from one recipe
    api_recipe = test_recipe.copy(update={"id": 100})
    api_cluster = test_cluster.copy(update={"id": 200, "recipe": api_recipe})
    api_collection = ObjectsCollection(
        recipes={api_recipe.arcadia_path: api_recipe},
        clusters={api_cluster.arcadia_path: api_cluster},
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    # 4. Check that only recipe has been changed, cluster has no changes
    assert len(diff.recipes.changed) == 1, "recipe has changed"
    assert diff.clusters.is_empty(), "cluster hasn't been changed, only recipe has"


def test_diff_builder_cluster_marked_as_changed_if_recipe_link_changed():
    # 1. Create another recipe. It differs by 'arcadia_path' field
    another_test_recipe = test_recipe.copy(update={"arcadia_path": "recipes/another/recipe.yaml"})

    # 2. IaC cluster has been created from recipes/meeseeks/recipe.yaml,
    # but now it is created from recipes/another/recipe.yaml
    iac_collection = ObjectsCollection(
        recipes={another_test_recipe.arcadia_path: another_test_recipe},
        clusters={test_cluster.arcadia_path: test_cluster.copy(update={"recipe": another_test_recipe})},
    )

    api_recipe = test_recipe.copy(update={"id": 100})
    api_cluster = test_cluster.copy(update={"id": 200, "recipe": api_recipe})
    api_collection = ObjectsCollection(
        recipes={api_recipe.arcadia_path: api_recipe},
        clusters={api_cluster.arcadia_path: api_cluster},
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    # 3. Check that cluster is marked as changed (because it's recipe link has been changed)
    assert len(diff.clusters.changed) == 1
    # ... new recipe has been added, one — deleted. We can not save recipe if 'arcadia_path' has been changed.
    assert len(diff.recipes.added) == len(diff.recipes.deleted) == 1


def test_diff_builder_prober_not_marked_as_changed_if_cluster_changed():
    # 1. Update cluster: change it's name
    changed_test_cluster = test_cluster.copy(update={"name": "New Name"})
    test_prober_config_for_cluster = test_prober_config.copy(update={"cluster": test_cluster})
    iac_collection = ObjectsCollection(
        recipes={test_recipe.arcadia_path: test_recipe},
        clusters={test_cluster.arcadia_path: changed_test_cluster},
        probers={test_prober.arcadia_path: test_prober.copy(update={"configs": [test_prober_config_for_cluster]})},
    )

    # 2. We have prober with config dependent from this cluster
    api_recipe = test_recipe.copy(update={"id": 100})
    api_cluster = test_cluster.copy(update={"id": 200, "recipe": api_recipe})
    api_prober_config = test_prober_config_for_cluster.copy(update={"id": 300})
    api_prober = test_prober.copy(update={"id": 300, "configs": [api_prober_config]})
    api_collection = ObjectsCollection(
        recipes={api_recipe.arcadia_path: api_recipe},
        clusters={api_cluster.arcadia_path: api_cluster},
        probers={api_prober.arcadia_path: api_prober},
    )

    diff = DiffBuilder().build(iac_collection, api_collection)

    # 3. Check that cluster has been changed
    assert len(diff.clusters.changed) == 1, "cluster has changed"
    # ... but prober has not been marked as changed
    assert diff.probers.is_empty(), "prober hasn't been changed, only cluster has"


def test_diff_builder_for_prober_variables():
    test_prober_config_copy = test_prober_config.copy(update={"variables": {"var1": "value1"}})
    api_prober_config = test_prober_config_copy.copy(update={"id": 300})
    api_prober = test_prober.copy(update={"id": 300, "configs": [api_prober_config]})
    iac_collection = ObjectsCollection(
        probers={test_prober.arcadia_path: test_prober.copy(update={"configs": [test_prober_config_copy]})}
    )

    # Check that variables is equal
    api_collection = ObjectsCollection(probers={api_prober.arcadia_path: api_prober})
    diff = DiffBuilder().build(iac_collection, api_collection)
    assert diff.is_empty()

    # Check that diff is not empty when variable changed
    api_collection = ObjectsCollection(
        probers={
            api_prober.arcadia_path: test_prober.copy(
                update={
                    "id": 300, "configs": [api_prober_config.copy(update={"variables": {"var1": "value2"}})]}
            )}
    )
    diff = DiffBuilder().build(iac_collection, api_collection)
    assert len(diff.probers.changed) == 1

    # Check that diff is not empty when variable appended
    api_collection = ObjectsCollection(
        probers={
            api_prober.arcadia_path: test_prober.copy(
                update={
                    "id": 300, "configs": [api_prober_config.copy(update={"variables": None})]}
            )}
    )
    diff = DiffBuilder().build(iac_collection, api_collection)
    assert len(diff.probers.changed) == 1

    # Check that diff is not empty when variable removed
    api_collection = ObjectsCollection(
        probers={
            api_prober.arcadia_path: test_prober.copy(update={"id": 300, "configs": [api_prober_config]})}
    )
    iac_collection = ObjectsCollection(
        probers={test_prober.arcadia_path: test_prober.copy(
            update={
                "configs": [test_prober_config_copy.copy(update={"variables": None})]}
        )}
    )
    diff = DiffBuilder().build(iac_collection, api_collection)
    assert len(diff.probers.changed) == 1

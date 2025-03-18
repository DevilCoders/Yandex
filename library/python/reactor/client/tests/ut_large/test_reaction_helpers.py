import logging
import os

import pytest

from reactor_client.helper.reaction.blank_reaction import BlankReactionBuilder
from reactor_client.helper.reaction.sandbox_reaction import SandboxReactionBuilder, SandboxPriorityClass, SandboxPrioritySubclass
from reactor_client import reactor_objects as r_objs
from . import get_path_for_tests, reactor_client, REACTOR_TEST_PROJECT_PATH, TEST_USER


_test_dir = get_path_for_tests("helpers")


@pytest.fixture(scope='module')
def client():
    return reactor_client()


def cleanup(reaction_paths, artifact_paths, client):
    client.namespace.delete([r_objs.NamespaceIdentifier(namespace_path=path) for path in reaction_paths], delete_if_exist=True)
    client.namespace.delete([r_objs.NamespaceIdentifier(namespace_path=path) for path in artifact_paths], delete_if_exist=True)
    client.namespace.delete([r_objs.NamespaceIdentifier(namespace_path=_test_dir)], delete_if_exist=True)


class HelperTest(object):
    def __init__(self, reaction_path, reaction_builder):
        """
        :type reaction_path: str
        :type reaction_builder: AbstractReactionBuilder
        """
        self.reaction_path = reaction_path
        self.reaction_builder = reaction_builder


class HelperTestCase(object):
    def __init__(self, helper_tests, artifacts=None):
        """
        :type helper_tests: list[HelperTest]
        :type artifacts: dict[str, r_objs.ArtifactTypeIdentifier] | None
        """
        self.helper_tests = helper_tests
        self.artifacts = artifacts


@pytest.fixture
def helper_test_case():
    """
    :rtype: HelperTestCase
    """
    blank_reaction_path = os.path.join(_test_dir, "blank")
    sandbox_reaction_path = os.path.join(_test_dir, "sandbox")
    int_input_path = os.path.join(_test_dir, "int_input")
    int_output_path = os.path.join(_test_dir, "int_output")
    return HelperTestCase(
        [
            HelperTest(
                blank_reaction_path,
                BlankReactionBuilder()
                    .set_reaction_path(blank_reaction_path)
                    .set_permissions({'@'+TEST_USER: r_objs.NamespaceRole.RESPONSIBLE, "reactor_group": r_objs.NamespaceRole.WRITER})
                    .set_project(REACTOR_TEST_PROJECT_PATH)
                    .set_reaction_instance_ttl_days(1)
                    .add_trigger_by_cron("cron", "0 0 0 * * ?", expr="global ut = Context.cronScheduledTime();")
                    .set_before_expression("global data = 42;")
                    .set_after_success_expression("a'" + int_output_path + "'.instantiate(data, ut);"),
            ),
            HelperTest(
                sandbox_reaction_path,
                SandboxReactionBuilder()
                    .set_reaction_path(sandbox_reaction_path)
                    .set_permissions({'@'+TEST_USER: r_objs.NamespaceRole.RESPONSIBLE, "reactor_group": r_objs.NamespaceRole.WRITER})
                    .set_project(REACTOR_TEST_PROJECT_PATH)
                    .set_reaction_instance_ttl_days(1)
                    .add_trigger_by_cron("cron", "0 0 0 * * ?", expr="global param1 = Context.cronScheduledTime().hour;")
                    .set_access_secret("robot-reactor_Sandbox_oauth")  # TODO(babichev-av) migrate to robot-reactor-tester@ when it becomes available
                    .set_task_type("TEST_TASK_2")
                    .set_owner("REACTORTESTGROUP")
                    .set_uniform_retries(2, 5000)
                    .set_task_ttl(100)
                    .set_ram(64)
                    .set_priority(SandboxPriorityClass.BACKGROUND, SandboxPrioritySubclass.LOW)
                    .add_acquiring_semaphore("REACTOR_TEST_SEMAPHORE", 6)
                    .set_before_expression("global param2 = 10;")
                    .add_input("fin_live_time", artifact_reference=r_objs.ArtifactReference(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=int_input_path)))
                    .add_input("live_time", const=15)
                    .add_input("prep_live_time", expression_var="param1")
                    .add_input("wait_time", expression_var="param2")
                    .add_output("result", expression_var="answer")
                    .set_after_success_expression("a'" + int_output_path + "'.instantiate(answer);"),
            ),
        ],
        {
            int_input_path: r_objs.ArtifactTypeIdentifier(artifact_type_key="PRIMITIVE_INT"),
            int_output_path: r_objs.ArtifactTypeIdentifier(artifact_type_key="PRIMITIVE_INT"),
        }
    )


def test_helpers(helper_test_case, client):
    try:
        for artifact_path, artifact_type_identifier in helper_test_case.artifacts.items():
            client.artifact.create(
                artifact_type_identifier=artifact_type_identifier,
                artifact_identifier=r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=artifact_path)),
                description="",
                permissions=r_objs.NamespacePermissions({TEST_USER: r_objs.NamespaceRole.RESPONSIBLE}),
                project_identifier=r_objs.ProjectIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=REACTOR_TEST_PROJECT_PATH)),
                cleanup_strategy=r_objs.CleanupStrategyDescriptor([r_objs.CleanupStrategy(r_objs.TtlCleanupStrategy(1))]),
                create_parent_namespaces=True,
                create_if_not_exist=True
            )
        for helper_test in helper_test_case.helper_tests:
            assert_create_reaction(helper_test, client)
    except:
        logging.error('Failed to init test case', exc_info=True)
        raise
    finally:
        cleanup([helper_test.reaction_path for helper_test in helper_test_case.helper_tests], helper_test_case.artifacts.keys(), client)


def assert_create_reaction(helper_test, client):
    """
    :type helper_test: HelperTest
    """
    reaction_reference = client.reaction.create(helper_test.reaction_builder.operation_descriptor, create_if_not_exist=True)
    client.reaction.get(operation_identifier=r_objs.OperationIdentifier(operation_id=reaction_reference.reaction_id))

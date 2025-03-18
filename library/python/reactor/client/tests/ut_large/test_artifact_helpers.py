from datetime import datetime
import logging
import os

import pytest

from reactor_client.helper.artifact_instance import ArtifactInstanceBuilder
from reactor_client import reactor_objects as r_objs
from . import get_path_for_tests, reactor_client, REACTOR_TEST_PROJECT_PATH, TEST_USER


_test_dir = get_path_for_tests("helpers")


@pytest.fixture(scope='module')
def client():
    return reactor_client()


def cleanup(artifact_namespace_identifiers, client):
    client.namespace.delete(artifact_namespace_identifiers, delete_if_exist=True)
    client.namespace.delete([r_objs.NamespaceIdentifier(namespace_path=_test_dir)], delete_if_exist=True)


class HelperTest(object):
    def __init__(self, artifact_type_key, artifact_instance_builder):
        """
        :type artifact_type_key: str
        :type artifact_instance_builder: ArtifactInstanceBuilder
        """
        self.artifact_type = r_objs.ArtifactTypeIdentifier(artifact_type_key=artifact_type_key)
        self.artifact_instance_builder = artifact_instance_builder


class HelperTestCase(object):
    def __init__(self, helper_tests):
        """
        :type helper_tests: list[HelperTest]
        """
        self.helper_tests = helper_tests


@pytest.fixture
def helper_tests():
    """
    :rtype: HelperTestCase
    """
    return HelperTestCase(
        [
            HelperTest(
                "EVENT",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "event"))
                    .set_event()
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_BOOL",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "bool"))
                    .set_bool(True)
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_INT",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "int"))
                    .set_int(42)
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_FLOAT",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "float"))
                    .set_float(1.0)
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_STRING",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "string"))
                    .set_string("aaa")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_LIST_BOOL",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "list_bool"))
                    .set_list_bool([True, False])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_LIST_INT",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "list_int"))
                    .set_list_int([])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_LIST_FLOAT",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "list_float"))
                    .set_list_float([1.0])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "PRIMITIVE_LIST_STRING",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "list_string"))
                    .set_list_string(["aaa", "bbb"])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "REFERENCE_URI",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "uri"))
                    .set_string("foo://bar/baz")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "REFERENCE_UUID",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "uuid"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_GRAPH_UUID",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_wf_uuid"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_GRAPH_INSTANCE_UUID",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_wfi_uuid"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_PARAMETER_RESOURCE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_resource"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_PARAMETER_LIST_RESOURCE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_list_resource"))
                    .set_list_string(["123e4567-e89b-12d3-a456-426655440000", "123e4567-e89b-12d3-a456-426655440000"])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_PARAMETER_SECRET",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_secret"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_PARAMETER_ENUM",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_enum"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_PARAMETER_LIST_ENUM",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_list_enum"))
                    .set_list_string(["123e4567-e89b-12d3-a456-426655440000", "123e4567-e89b-12d3-a456-426655440000"])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_TSV",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_tsv"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_TEXT",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_txt"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_HTML",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_html"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_XML",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_xml"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_IMAGE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_img"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_BINARY",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_bin"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_EXECUTABLE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_exe"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FILE_JSON",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_json"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_MR_TABLE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_mr_table"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_MR_FILE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_mr_file"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_MR_DIRECTORY",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_mr_dir"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_HIVE_TABLE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_hive_table"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FML_FORMULA",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_fml_formula"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FML_FORMULA_SERP_PREFS",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_fml_formula_serp_prefs"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FML_SERP_COMPARISON",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_fml_serp_comp"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FML_POOL",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_fml_pool"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FML_PRS",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_fml_prs"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "NIRVANA_DATA_FML_DUMP_PARSE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "nirvana_fml_dump_parse"))
                    .set_string("123e4567-e89b-12d3-a456-426655440000")
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "SANDBOX_RESOURCE",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "sandbox_resource"))
                    .set_sandbox_resource(1928331394)
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "YT_PATH",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "yt_path"))
                    .set_yt_path('hahn', 'path/to/table')
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
            HelperTest(
                "YT_LIST_PATH",
                ArtifactInstanceBuilder()
                    .set_artifact_path(os.path.join(_test_dir, "yt_list_path"))
                    .set_list_yt_path([{'cluster': 'hahn', 'path': 'path/to/table1'}, {'cluster': 'hahn', 'path': 'path/to/table2'}])
                    .set_user_time(datetime(2020, 1, 1))
                    .set_attributes({'tag1': 'value1', 'tag2': 'value2'}),
            ),
        ],
    )


def test_helpers(helper_tests, client):
    try:
        for helper_test in helper_tests.helper_tests:
            builder = helper_test.artifact_instance_builder
            client.artifact.create(
                artifact_type_identifier=helper_test.artifact_type,
                artifact_identifier=builder.artifact_identifier,
                description="",
                permissions=r_objs.NamespacePermissions({TEST_USER: r_objs.NamespaceRole.RESPONSIBLE}),
                project_identifier=r_objs.ProjectIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=REACTOR_TEST_PROJECT_PATH)),
                cleanup_strategy=r_objs.CleanupStrategyDescriptor([r_objs.CleanupStrategy(r_objs.TtlCleanupStrategy(1))]),
                create_parent_namespaces=True,
                create_if_not_exist=True
            )
            response = client.artifact_instance.instantiate(builder=builder)
            assert response.artifact_instance_id
    except:
        logging.error('Failed to init test case', exc_info=True)
        raise
    finally:
        cleanup([helper_test.artifact_instance_builder.artifact_identifier.namespace_identifier for helper_test in helper_tests.helper_tests], client)

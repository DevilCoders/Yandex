import abc
import logging
import os
from typing import List
from typing import Optional
from typing import Tuple

import vh
from library.python import resource

from cloud.dwh.nirvana import reactor
from cloud.dwh.nirvana.vh.common.operations import ANY_TO_MR_TABLE
from cloud.dwh.nirvana.vh.common.operations import IF_BRANCHING_CONDITION
from cloud.dwh.nirvana.vh.common.operations import IF_BRANCHING_MR_PATH_EXISTS
from cloud.dwh.nirvana.vh.common.operations import get_default_copy_mr_table_op
from cloud.dwh.nirvana.vh.common.operations import get_default_get_mr_directory_op
from cloud.dwh.nirvana.vh.common.operations import get_default_get_mr_table_op
from cloud.dwh.nirvana.vh.common.operations import get_default_mr_drop_op
from cloud.dwh.nirvana.vh.common.operations import get_default_yql_2_inputs_op
from cloud.dwh.nirvana.vh.common.operations import get_default_yql_op
from cloud.dwh.nirvana.vh.common.operations import get_disable_cache_op
from cloud.dwh.nirvana.vh.common.operations import get_yql_utils_library_archive_op
from cloud.dwh.nirvana.vh.common.workflows.base import InputTriggerBlockReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows.base import ReactiveWorkflowBase
from cloud.dwh.nirvana.vh.common.workflows.lag_monitor import LagMonitoringWorkflowBase
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig

LOG = logging.getLogger(__name__)


class YqlWorkflowMixin:

    def _get_yql_query_utils_files(self) -> List[str]:
        """
        YQL extra utils related only to the workflow. Can be used as import
        at `query.sql` in order to decompose complexity of the file.

        PRAGMA Library("your_utils_file.sql");
        IMPORT `your_utils_file` SYMBOLS $get_your_function;
        """
        return []

    def _get_yql_utils_library_archive_op(self):
        """
        Returns Nirvana operation call to make archive with YQL files to import
        """
        return get_yql_utils_library_archive_op(include_sql_files=self._get_yql_query_utils_files())


class SingleYqlReactiveWorkflowBase(LagMonitoringWorkflowBase, ReactiveWorkflowBase, YqlWorkflowMixin, abc.ABC):
    """
    Run yql.
    Get yql query from `./resources/query.sql` and execute it.
    Parameters for query can be set in `./resources/parameters.yaml` in key `query_parameters`

    Parameters (from file `parameters.yaml`):
    * `query_parameters` - dictionary with query parameters. In query you can use it like `$key = {{param["key"]->quote()}}`
    * `destination_path` - YT destination path. Can be omitted. If it set, artifact result will be that value. In query you can use it like `$destination_path = {{input1->table_quote()}};`
    """

    def _get_yql_query(self) -> str:
        """
        YQL query from `./resources/query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'query.sql'))

    def _get_yql_query_parameters(self, deploy_config: DeployConfig) -> dict:
        """
        YQL query parameters  from `./resources/parameters.yaml:{environment}:query_parameters`
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('query_parameters', {})

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is `./resources/parameters.yaml:{environment}:destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('destination_path', '')

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        parameters = self._get_environment_parameters(environment=deploy_config.environment)

        if 'destination_path' in parameters:
            output_mr_table_op = get_default_get_mr_table_op(deploy_config)
            output_mr_table_result = output_mr_table_op(
                _name='Output table',
                _options={
                    'table': parameters['destination_path'],
                },
            )
        else:
            output_mr_table_result = None

        yql_utils_archive_op = self._get_yql_utils_library_archive_op()

        yql_op = get_default_yql_op(deploy_config)
        yql_op_result = yql_op(
            _name='Run query',
            _inputs={
                'input1': output_mr_table_result and output_mr_table_result['outTable'],
                'files': [yql_utils_archive_op['archive']],
            },
            _options={
                'request': self._get_yql_query(),
                'param': [f'{name}={param}' for name, param in self._get_yql_query_parameters(deploy_config).items()],
                'ttl': parameters.get('ttl')
            },
        )

        # lag monitor
        self.maybe_add_lag_monitor(deploy_config=deploy_config, after=yql_op_result, parameters=parameters)


class CreateOrUpdateYqlReactiveWorkflowBase(ReactiveWorkflowBase, YqlWorkflowMixin, abc.ABC):
    """
    Run yql with destination table creation or if destination table exists, run yql with destination table update.
    Get yql create query from `./resources/create_query.sql`.
    Get yql update query from `./resources/update_query.sql`.
    Parameters for query can be set in `./resources/parameters.yaml` in key `query_parameters`

    Parameters (from file `parameters.yaml`):
    * `query_parameters` - dictionary with query parameters. In query you can use it like `$key = {{param["key"]->quote()}}`
    * `destination_path` - YT destination path. Artifact result will be that value. In query you can use it like `$destination_path = {{input1->table_quote()}};`
    """

    def _get_create_yql_query(self) -> str:
        """
        YQL create query from `./resources/create_query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'create_query.sql'))

    def _get_update_yql_query(self) -> str:
        """
        YQL update query from `./resources/update_query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'update_query.sql'))

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is `./resources/parameters.yaml:{environment}:destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment)['destination_path']

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        parameters = self._get_environment_parameters(environment=deploy_config.environment)

        output_mr_table_op = get_default_get_mr_table_op(deploy_config)
        output_mr_table_result = output_mr_table_op(
            _name='Output table',
            _options={
                'table': parameters['destination_path'],
            },
        )

        if_mr_path_exists_op = vh.op(id=IF_BRANCHING_MR_PATH_EXISTS)
        if_mr_path_exists_result = if_mr_path_exists_op(
            _name='Check output table exists',
            _inputs={
                'path': output_mr_table_result['outTable'],
            },
            _options={
                'yt-token': deploy_config.yt_token,
            },
        )

        if_condition_op = vh.op(id=IF_BRANCHING_CONDITION)
        if_condition_result = if_condition_op(
            _name='If .. else ..',
            _inputs={
                'data': output_mr_table_result['outTable'],
            },
            _options={
                'condition': False,  # not real value
            },
            _dynamic_options=if_mr_path_exists_result,
        )

        # if table exists
        mr_table_existing_op = vh.op(id=ANY_TO_MR_TABLE)
        mr_table_existing_result = mr_table_existing_op(
            _name='if output table exists',
            _inputs={
                'File': if_condition_result['true'],
            },
        )

        yql_utils_archive_op = self._get_yql_utils_library_archive_op()

        yql_update_op = get_default_yql_op(deploy_config)
        yql_update_op(
            _name='Update query',
            _inputs={
                'input1': mr_table_existing_result['mr_file'],
                'files': [yql_utils_archive_op['archive']],
            },
            _options={
                'request': self._get_update_yql_query(),
                'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
            },
            _after=[mr_table_existing_result],
        )

        # if table not exists
        mr_table_not_existing_op = vh.op(id=ANY_TO_MR_TABLE)
        mr_table_not_existing_result = mr_table_not_existing_op(
            _name='if output table does not exist',
            _inputs={
                'File': if_condition_result['false'],
            },
        )

        yql_create_op = get_default_yql_op(deploy_config)
        yql_create_op(
            _name='Create query',
            _inputs={
                'input1': mr_table_not_existing_result['mr_file'],
                'files': [yql_utils_archive_op['archive']],
            },
            _options={
                'request': self._get_create_yql_query(),
                'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
            },
            _after=[mr_table_not_existing_result],
        )


class SingleYqlWithChecksReactiveWorkflowBase(LagMonitoringWorkflowBase, ReactiveWorkflowBase, YqlWorkflowMixin,
                                              abc.ABC):
    """
    Run yql, run yql query with checks, copy temporary table to main
    Get yql query from `./resources/query.sql`.
    Get yql check queries from `./resources/check_*.sql`.
    Parameters for query can be set in `./resources/parameters.yaml` in key `query_parameters`

    Parameters (from file `parameters.yaml`):
    * `query_parameters` - dictionary with query parameters. In query you can use it like `$key = {{param["key"]->quote()}}`
    * `destination_path` - YT destination path. Artifact result will be that value. In query you can use it like `$destination_path = {{input1->table_quote()}};`
    """

    def _get_temporary_destination_table_path(self, deploy_config: DeployConfig) -> str:
        """
        Temporary table path
        """
        return os.path.join(deploy_config.yt_tmp_layer_path, self._relative_module_path, 'table')

    def _get_yql_query(self) -> str:
        """
        YQL query from `./resources/query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'query.sql'))

    def _get_check_queries(self) -> List[Tuple[str, str]]:
        """
        YQL check queries from `./resources/check_*.sql`
        """
        return resource.iteritems(prefix=os.path.join(self._resource_folder, 'check_'), strip_prefix=True)

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is `./resources/parameters.yaml:{environment}:destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment)['destination_path']

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        parameters = self._get_environment_parameters(environment=deploy_config.environment)

        # Create temporary table

        temporary_destination_path = self._get_temporary_destination_table_path(deploy_config)

        temporary_mr_table_op = get_default_get_mr_table_op(deploy_config)
        temporary_mr_table_result = temporary_mr_table_op(
            _name='Temporary table',
            _options={
                'table': temporary_destination_path,
            },
        )

        # Disable cache

        disable_cache_op_result = get_disable_cache_op()

        # Generate temporary table

        yql_utils_archive_op = self._get_yql_utils_library_archive_op()

        temporary_yql_op = get_default_yql_op(deploy_config, disable_cache_op_result=disable_cache_op_result)
        temporary_yql_op_result = temporary_yql_op(
            _name='Generate temporary table',
            _inputs={
                'input1': temporary_mr_table_result['outTable'],
                'files': [yql_utils_archive_op['archive']],
            },
            _options={
                'request': self._get_yql_query(),
                'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
                'ttl': parameters.get('ttl')
            },
        )

        # Validate temporary table

        check_op_results = []
        for name, query in self._get_check_queries():
            yql_op = get_default_yql_op(deploy_config, disable_cache_op_result=disable_cache_op_result)
            yql_op_result = yql_op(
                _name=f'Validate temporary table - {name.removesuffix(".sql")}',
                _inputs={
                    'input1': temporary_mr_table_result['outTable'],
                    'files': [yql_utils_archive_op['archive']],
                },
                _options={
                    'request': query,
                    'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
                    'ttl': parameters.get('ttl')
                },
                _after=[temporary_yql_op_result]
            )

            check_op_results.append(yql_op_result)

        # Copy from temporary to destination

        copy_table_op = get_default_copy_mr_table_op(deploy_config)
        copy_table_op_result = copy_table_op(
            _name='Copy from temporary to destination',
            _inputs={
                'src': temporary_mr_table_result['outTable'],
            },
            _options={
                'dst-path': parameters['destination_path'],
            },
            _dynamic_options=disable_cache_op_result,
            _after=check_op_results,
        )

        # Delete temporary table
        mr_drop_op = get_default_mr_drop_op(deploy_config)
        mr_drop_op(
            _name='Drop temporary',
            _inputs={
                'paths': temporary_mr_table_result['outTable'],
            },
            _dynamic_options=disable_cache_op_result,
            _after=[copy_table_op_result],
        )

        # lag monitor
        self.maybe_add_lag_monitor(deploy_config=deploy_config, after=copy_table_op_result, parameters=parameters)


class InputTriggerMrDirectorySingleYqlReactiveWorkflowBase(InputTriggerBlockReactiveWorkflowBase, YqlWorkflowMixin,
                                                           abc.ABC):
    """
    Run yql using input trigger artifact result as MR Directory
    Get yql query from `./resources/query.sql`.
    Parameters for query can be set in `./resources/parameters.yaml` in key `query_parameters`

    Parameters (from file `parameters.yaml`):
    * `query_parameters` - dictionary with query parameters. In query you can use it like `$key = {{param["key"]->quote()}}`
    * `input_triggers` - Input artifact path trigger where result is mr directory path. In query you can use it like `$directory = {{input_dir->table_quote()}}`
    * `destination_path` - YT destination path. Can be omitted. Artifact result will be that value. In query you can use it like `$destination_path = {{input1->table_quote()}};`
    """

    def _get_input_triggers_type(self) -> Optional[reactor.InputTriggerType]:
        """
        Reaction input triggers type
        """
        return reactor.InputTriggerType.ALL

    def _get_input_trigger_block_param_name(self):
        """
        `Get Mr Directory` operation parameter
        """
        return 'path'

    def _get_yql_query(self) -> str:
        """
        YQL query from `./resources/query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'query.sql'))

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is ``
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('destination_path', '')

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        parameters = self._get_environment_parameters(environment=deploy_config.environment)

        block_codes_by_artifact = self._get_block_codes_by_artifact(deploy_config=deploy_config)

        if len(block_codes_by_artifact) != 1:
            raise RuntimeError(f'There is more or less than just one artifact: {block_codes_by_artifact.keys()}')

        output_mr_directory_op = get_default_get_mr_directory_op(deploy_config)
        output_mr_directory_result = output_mr_directory_op(
            _name='Input directory',
            _code=list(block_codes_by_artifact.values())[0],
            _options={
                'creationMode': 'CHECK_EXISTS',
                'path': '',  # from artifact
            },
        )

        if 'destination_path' in parameters:
            output_mr_table_op = get_default_get_mr_table_op(deploy_config)
            output_mr_table_result = output_mr_table_op(
                _name='Output table',
                _options={
                    'table': parameters['destination_path'],
                },
            )
        else:
            output_mr_table_result = None

        yql_utils_archive_op = self._get_yql_utils_library_archive_op()

        yql_op = get_default_yql_op(deploy_config)
        yql_op(
            _name='Run query',
            _inputs={
                'input1': output_mr_table_result and output_mr_table_result['outTable'],
                'files': [yql_utils_archive_op['archive'], output_mr_directory_result['mr_directory']],
            },
            _options={
                'request': self._get_yql_query(),
                'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
                'ttl': parameters.get('ttl')
            },
        )


class InputTriggerMultiMrTableSingleYqlReactiveWorkflowBase(InputTriggerBlockReactiveWorkflowBase,
                                                            LagMonitoringWorkflowBase, YqlWorkflowMixin, abc.ABC):
    """
    Run yql using input trigger artifacts results as MR Table (can be multiple inputs)
    Get yql query from `./resources/query.sql`.
    Parameters for query can be set in `./resources/parameters.yaml` in key `query_parameters`

    Parameters (from file `parameters.yaml`):
    * `query_parameters` - dictionary with query parameters. In query you can use it like `$key = {{param["key"]->quote()}}`
    * `input_triggers` - Input artifacts path triggers where results are mr table path. In query you can use it like `$directory = {{concat_input2}}`
    * `destination_path` - YT destination path. Can be omitted. Artifact result will be that value. In query you can use it like `$destination_path = {{input1->table_quote()}};`
    """

    def _get_input_trigger_block_param_name(self):
        """
        `Get Mr Table` operation parameter
        """
        return 'table'

    def _get_yql_query(self) -> str:
        """
        YQL query from `./resources/query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'query.sql'))

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is `./resources/parameters.yaml:{environment}:destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('destination_path', '')

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        parameters = self._get_environment_parameters(environment=deploy_config.environment)

        mr_tables = []
        block_codes_by_artifact = self._get_block_codes_by_artifact(deploy_config=deploy_config)

        for artifact, block_code in block_codes_by_artifact.items():
            input_mr_table_op = get_default_get_mr_table_op(deploy_config)
            input_mr_table_op_result = input_mr_table_op(
                _name=artifact.path,
                _code=block_code,
                _options={
                    'table': '',  # from artifact
                },
            )

            mr_tables.append(input_mr_table_op_result['outTable'])

        if 'destination_path' in parameters:
            output_mr_table_op = get_default_get_mr_table_op(deploy_config)
            output_mr_table_result = output_mr_table_op(
                _name='Output table',
                _options={
                    'table': parameters['destination_path'],
                },
            )
        else:
            output_mr_table_result = None

        yql_utils_archive_op = self._get_yql_utils_library_archive_op()

        yql_op = get_default_yql_2_inputs_op(deploy_config)
        yql_op_result = yql_op(
            _name='Run query',
            _inputs={
                'input1': output_mr_table_result and output_mr_table_result['outTable'],
                'input2': mr_tables,
                'files': [yql_utils_archive_op['archive']],
            },
            _options={
                'request': self._get_yql_query(),
                'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
                'ttl': parameters.get('ttl')
            }
        )

        self.maybe_add_lag_monitor(deploy_config=deploy_config, after=yql_op_result, parameters=parameters)


class InputTriggerMultiMrTableSingleYqlWithChecksReactiveWorkflowBase(InputTriggerBlockReactiveWorkflowBase,
                                                                      YqlWorkflowMixin, abc.ABC):
    """
    Run yql using input trigger artifacts results as MR Table (can be multiple inputs)
    Run yql query with checks, copy temporary table to main
    Get yql query from `./resources/query.sql`.
    Parameters for query can be set in `./resources/parameters.yaml` in key `query_parameters`

    Parameters (from file `parameters.yaml`):
    * `query_parameters` - dictionary with query parameters. In query you can use it like `$key = {{param["key"]->quote()}}`
    * `input_triggers` - Input artifacts path triggers where results are mr table path. In query you can use it like `$directory = {{concat_input2}}`
    * `destination_path` - YT destination path. Can be omitted. Artifact result will be that value. In query you can use it like `$destination_path = {{input1->table_quote()}};`
    """

    def _get_input_trigger_block_param_name(self):
        """
        `Get Mr Table` operation parameter
        """
        return 'table'

    def _get_yql_query(self) -> str:
        """
        YQL query from `./resources/query.sql`
        """
        return resource.find(os.path.join(self._resource_folder, 'query.sql'))

    def _get_artifact_success_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        Artifact success result is `./resources/parameters.yaml:{environment}:destination_path`
        """
        return self._get_environment_parameters(environment=deploy_config.environment).get('destination_path', '')

    def _get_artifact_failure_result(self, deploy_config: DeployConfig) -> Optional[str]:
        """
        No artifact on failure
        """
        return None

    def _get_temporary_destination_table_path(self, deploy_config: DeployConfig) -> str:
        """
        Temporary table path
        """
        return os.path.join(deploy_config.yt_tmp_layer_path, self._relative_module_path, 'table')

    def _get_check_queries(self) -> List[Tuple[str, str]]:
        """
        YQL check queries from `./resources/check_*.sql`
        """
        return resource.iteritems(prefix=os.path.join(self._resource_folder, 'check_'), strip_prefix=True)

    def _create_artifacts(self, deploy_config: DeployConfig):
        mr_tables = []
        block_codes_by_artifact = self._get_block_codes_by_artifact(deploy_config=deploy_config)

        for artifact, block_code in block_codes_by_artifact.items():
            input_mr_table_op = get_default_get_mr_table_op(deploy_config)
            input_mr_table_op_result = input_mr_table_op(
                _name=artifact.path,
                _code=block_code,
                _options={
                    'table': '',  # from artifact
                },
            )

            mr_tables.append(input_mr_table_op_result['outTable'])
        return mr_tables

    def _create_temporary_table(self, deploy_config: DeployConfig, temporary_destination_path: str):
        temporary_mr_table_op = get_default_get_mr_table_op(deploy_config)
        temporary_mr_table_result = temporary_mr_table_op(
            _name='Temporary table',
            _options={
                'table': temporary_destination_path,
            },
        )
        return temporary_mr_table_result

    def _generate_temporary_table(self, deploy_config: DeployConfig, disable_cache_op_result, parameters,
                                  temporary_mr_table_result, mr_tables, yql_utils_archive_op):

        temporary_yql_op = get_default_yql_2_inputs_op(deploy_config, disable_cache_op_result=disable_cache_op_result)
        temporary_yql_op_result = temporary_yql_op(
            _name='Run query',
            _inputs={
                'input1': temporary_mr_table_result['outTable'],
                'input2': mr_tables,
                'files': [yql_utils_archive_op['archive']],
            },
            _options={
                'request': self._get_yql_query(),
                'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
                'ttl': parameters.get('ttl')
            },
        )
        return temporary_yql_op_result

    def _validate_temporary_table(self, deploy_config: DeployConfig, disable_cache_op_result, temporary_mr_table_result,
                                  yql_utils_archive_op, temporary_yql_op_result, parameters):
        check_op_results = []
        for name, query in self._get_check_queries():
            yql_op = get_default_yql_op(deploy_config, disable_cache_op_result=disable_cache_op_result)
            yql_op_result = yql_op(
                _name=f'Validate temporary table - {name.removesuffix(".sql")}',
                _inputs={
                    'input1': temporary_mr_table_result['outTable'],
                    'files': [yql_utils_archive_op['archive']],
                },
                _options={
                    'request': query,
                    'param': [f'{name}={param}' for name, param in parameters.get('query_parameters', {}).items()],
                    'ttl': parameters.get('ttl')
                },
                _after=[temporary_yql_op_result]
            )

            check_op_results.append(yql_op_result)
        return check_op_results

    def _copy_to_destination_table(self, deploy_config: DeployConfig, temporary_mr_table_result,
                                   disable_cache_op_result, parameters, check_op_results):
        copy_table_op = get_default_copy_mr_table_op(deploy_config)
        copy_table_op_result = copy_table_op(
            _name='Copy from temporary to destination',
            _inputs={
                'src': temporary_mr_table_result['outTable'],
            },
            _options={
                'dst-path': parameters['destination_path'],
            },
            _dynamic_options=disable_cache_op_result,
            _after=check_op_results,
        )
        return copy_table_op_result

    def _drop_temporary_table(self, deploy_config: DeployConfig, temporary_mr_table_result, disable_cache_op_result,
                              copy_table_op_result):
        mr_drop_op = get_default_mr_drop_op(deploy_config)
        mr_drop_op_result = mr_drop_op(
            _name='Drop temporary',
            _inputs={
                'paths': temporary_mr_table_result['outTable'],
            },
            _dynamic_options=disable_cache_op_result,
            _after=[copy_table_op_result],
        )
        return mr_drop_op_result

    def _fill_graph(self, graph: vh.Graph, deploy_config: DeployConfig):
        """
        Fill graph with operations.
        """

        parameters = self._get_environment_parameters(environment=deploy_config.environment)
        mr_tables = self._create_artifacts(deploy_config=deploy_config)

        temporary_destination_path = self._get_temporary_destination_table_path(deploy_config)
        temporary_mr_table_result = self._create_temporary_table(deploy_config=deploy_config,
                                                                 temporary_destination_path=temporary_destination_path)

        disable_cache_op_result = get_disable_cache_op()
        yql_utils_archive_op = self._get_yql_utils_library_archive_op()

        temporary_yql_op_result = self._generate_temporary_table(deploy_config=deploy_config,
                                                                 disable_cache_op_result=disable_cache_op_result,
                                                                 temporary_mr_table_result=temporary_mr_table_result,
                                                                 mr_tables=mr_tables,
                                                                 parameters=parameters,
                                                                 yql_utils_archive_op=yql_utils_archive_op)
        check_op_results = self._validate_temporary_table(deploy_config=deploy_config,
                                                          disable_cache_op_result=disable_cache_op_result,
                                                          temporary_mr_table_result=temporary_mr_table_result,
                                                          yql_utils_archive_op=yql_utils_archive_op,
                                                          temporary_yql_op_result=temporary_yql_op_result,
                                                          parameters=parameters)

        copy_table_op_result = self._copy_to_destination_table(deploy_config=deploy_config,
                                                               temporary_mr_table_result=temporary_mr_table_result,
                                                               disable_cache_op_result=disable_cache_op_result,
                                                               parameters=parameters,
                                                               check_op_results=check_op_results)
        self._drop_temporary_table(deploy_config=deploy_config,
                                   temporary_mr_table_result=temporary_mr_table_result,
                                   disable_cache_op_result=disable_cache_op_result,
                                   copy_table_op_result=copy_table_op_result)

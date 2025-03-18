# -*- coding: utf-8 -*-
import contextlib
import itertools
import numbers
import os

from typing import Any
from typing import Dict
from typing import Generator
from typing import List
from typing import Optional
from typing import Set

import session_local.mr_merge_local as merge_local
import session_local.versions_local as versions_local
import session_squeezer.services as squeezer_services
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix

from mstand_enums.mstand_online_enums import ServiceSourceEnum
from mstand_structs.squeeze_versions import SqueezeVersions
from session_squeezer.experiment_for_squeeze import ExperimentForSqueeze
from session_squeezer.squeeze_runner import SqueezeTask
from mstand_utils.mstand_paths_utils import SqueezePaths
from session_squeezer.squeezer import UserSessionsSqueezer


class SqueezeLocalReducer(object):
    SCHEMA_TYPE_2_EXPECTED_TYPE = {
        "boolean": bool,
        "int32": numbers.Integral,
        "int64": numbers.Integral,
        "double": numbers.Real,
        "string": str,
        "any": object,  # Will it work?
    }

    def __init__(self, squeezer, destinations, table_types):
        self.squeezer = squeezer
        self.destinations = destinations
        self.table_types = table_types
        self.assert_table_index = False

    def run(self, paths):
        files = {exp: ufile.fopen_write(dst, use_unicode=False)
                 for (exp, dst) in usix.iteritems(self.destinations)}

        rows = merge_local.merge_iterators(
            [_iter_key_value_file(path) for path in paths],
            keys=("key", "subkey"),
        )

        for yuid, group in itertools.groupby(rows, key=lambda r: r["key"]):
            if self.assert_table_index:
                group = list(group)
                indices = [g["@table_index"] for g in group]
                assert indices == sorted(indices)

            for exp, action in self.squeezer.squeeze_session(yuid, group, self.table_types):
                fd = files[exp]
                service = exp.service
                SqueezeLocalReducer._validate_and_add_null(
                    action,
                    service,
                    SqueezeLocalReducer.SCHEMA_TYPE_2_EXPECTED_TYPE,
                )
                ujson.dump_to_fd(action, fd, sort_keys=True)
                fd.write("\n")

        for fd in usix.itervalues(files):
            fd.close()

    @staticmethod
    def _validate_and_add_null(mutable_action, service, schema_type_2_expected_type):
        columns_description = squeezer_services.get_columns_description_by_service(service)

        name_2_schema_type = {cd["name"]: cd["type"] for cd in columns_description}

        columns_not_at_scheme = set(mutable_action) - set(name_2_schema_type)
        if columns_not_at_scheme:
            MSTAND_1113_set = {
                "actions_will_be_copied_to",
                "fork_uid_produce_actions",
                "actions_was_copied_from",
                "fork_banner_id",
            }
            if len(columns_not_at_scheme - MSTAND_1113_set):
                raise ValueError("Unexpected columns {0}.".format(columns_not_at_scheme))
            else:
                pass

        for key, value in usix.iteritems(mutable_action):
            if value is not None:
                key_schema_type = name_2_schema_type[key]
                if key_schema_type in schema_type_2_expected_type:
                    if not isinstance(value, schema_type_2_expected_type[key_schema_type]):
                        raise ValueError("Bad type for {0}. Expected {1}".format(key, key_schema_type))

                else:
                    raise ValueError("Unknown type for validation", key_schema_type)

        # add null aka None
        for missing_column in (set(name_2_schema_type) - set(mutable_action)):
            mutable_action[missing_column] = None


def _iter_key_value_file(path):
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                obj = ujson.load_from_str(line)
                yield obj


def squeeze_day_impl(
        destinations,
        squeezer,
        paths,
        versions,
        task,
):
    """
    :type destinations: dict[ExperimentForSqueeze, str]
    :type squeezer: session_squeezer.squeezer.UserSessionsSqueezer
    :type paths: mstand_utils.mstand_paths_utils.SqueezePaths
    :type versions: SqueezeVersions
    :type task: SqueezeTask
    """

    assert task.source in ServiceSourceEnum.USER_SESSIONS_LOGS, \
        "can't use {} in local mode".format(task.source)

    reducer = SqueezeLocalReducer(squeezer, destinations, paths.types)
    reducer.run(paths.tables)
    for exp, path in usix.iteritems(destinations):
        with_history = exp.all_for_history
        with_filters = bool(exp.observation.filters)
        table_versions = versions.clone(service=exp.service, with_history=with_history, with_filters=with_filters)
        versions_local.set_versions(path, table_versions)


# noinspection PyMethodMayBeStatic
class SqueezeBackendLocal:
    def __init__(self):
        pass

    def get_existing_paths(self, paths: List[str]) -> Generator[str, None, None]:
        return (path for path in paths if os.path.isfile(path))

    def get_all_versions(self, paths: List[str]) -> Dict[str, SqueezeVersions]:
        return versions_local.get_all_versions(paths)

    def prepare_dirs(self, paths: Set[str]) -> None:
        for dir_path in paths:
            ufile.make_dirs(dir_path)

    def squeeze_one_day(
            self,
            destinations: Dict[ExperimentForSqueeze, str],
            squeezer: UserSessionsSqueezer,
            paths: SqueezePaths,
            versions: SqueezeVersions,
            task: SqueezeTask,
            operation_sid: str,
            enable_binary: bool,
    ) -> Any:

        return squeeze_day_impl(
            destinations=destinations,
            squeezer=squeezer,
            paths=paths,
            versions=versions,
            task=task,
        )

    @contextlib.contextmanager
    def lock(self, destinations: Dict[ExperimentForSqueeze, str]) -> Generator[None, None, None]:
        yield

    def get_cache_checker(self, min_versions: SqueezeVersions, need_replace: bool) -> None:
        pass

    def write_log(self, rectype: str, data: Optional[Dict[str, Any]] = None) -> None:
        pass

    def get_cache_tables(self, ready_tables: Set[str]) -> Dict[str, Any]:
        pass

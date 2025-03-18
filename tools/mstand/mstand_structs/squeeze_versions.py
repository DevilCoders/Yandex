import sys
import yaqutils.nirvana_helpers as unirvana
import yaqutils.six_helpers as usix

from mstand_enums.mstand_general_enums import SqueezeWayEnum  # noqa


class SqueezeVersions(object):
    def __init__(self, service_versions=None, common=None, history=None, filters=None,
                 python_version=None, revision=None, workflow_url=None, squeeze_way=None):
        """
        :type service_versions: dict
        :type common: int | None
        :type history: int | None
        :type filters: int | None
        :type python_version: str | None
        :type revision: int | None
        :type workflow_url: str | None
        :type squeeze_way: SqueezeWayEnum | None
        """
        if service_versions is None:
            service_versions = {}
        self.service_versions = service_versions
        self.common = common
        self.history = history
        self.filters = filters
        if python_version is None:
            python_version = get_python_version()
        self.python_version = python_version
        if revision is None:
            revision = get_revision()
        self.revision = revision
        if workflow_url is None:
            workflow_url = unirvana.get_nirvana_workflow_url()
        self.workflow_url = workflow_url
        self.squeeze_way = squeeze_way

    def clone(self, service=None, with_history=True, with_filters=True, squeeze_way=None):
        """
        :type service: str | None
        :type with_history: bool
        :type with_filters: bool
        :type squeeze_way: SqueezeWayEnum | None
        :rtype: SqueezeVersions
        """
        if service is None:
            service_versions = self.service_versions
        else:
            service_versions = {service: self.service_versions.get(service)}

        history = self.history if with_history else None
        filters = self.filters if with_filters else None

        return SqueezeVersions(
            service_versions=service_versions,
            common=self.common,
            history=history,
            filters=filters,
            python_version=self.python_version,
            revision=self.revision,
            workflow_url=self.workflow_url,
            squeeze_way=squeeze_way,
        )

    def serialize(self):
        """
        :rtype: dict[str, int]
        """
        result = dict(self.service_versions)
        if self.common:
            result[SQUEEZE_COMMON_FIELD_NAME] = self.common
        if self.history:
            result[SQUEEZE_HISTORY_FIELD_NAME] = self.history
        if self.filters:
            result[SQUEEZE_FILTERS_FIELD_NAME] = self.filters
        if self.python_version:
            result[SQUEEZE_PYTHON_FIELD_NAME] = self.python_version
        if self.revision:
            result[SQUEEZE_REVISION_FIELD_NAME] = self.revision
        if self.workflow_url:
            result[SQUEEZE_WORKFLOW_URL_FIELD_NAME] = self.workflow_url
        if self.squeeze_way:
            result[SQUEEZE_WAY_FIELD_NAME] = self.squeeze_way
        return result

    @staticmethod
    def deserialize(data, update_workflow_url=False):
        """
        :type data: dict
        :type update_workflow_url: bool
        :rtype: SqueezeVersions
        """
        service_versions = dict(data)
        common = service_versions.pop(SQUEEZE_COMMON_FIELD_NAME, None)
        history = service_versions.pop(SQUEEZE_HISTORY_FIELD_NAME, None)
        filters = service_versions.pop(SQUEEZE_FILTERS_FIELD_NAME, None)
        python_version = service_versions.pop(SQUEEZE_PYTHON_FIELD_NAME, None)
        revision = service_versions.pop(SQUEEZE_REVISION_FIELD_NAME, None)
        workflow_url = service_versions.pop(SQUEEZE_WORKFLOW_URL_FIELD_NAME, None)
        squeeze_way = service_versions.pop(SQUEEZE_WAY_FIELD_NAME, None)
        if update_workflow_url:
            workflow_url = None
        return SqueezeVersions(
            service_versions=service_versions,
            common=common,
            history=history,
            filters=filters,
            python_version=python_version,
            revision=revision,
            workflow_url=workflow_url,
            squeeze_way=squeeze_way,
        )

    def __str__(self):
        return str(self.serialize())

    def __repr__(self):
        return str(self)

    def is_empty(self):
        return not self.service_versions and not self.common and not self.history and not self.filters

    def is_older_than(self, other, check_history=True):
        """
        :type other: SqueezeVersions
        :type check_history: bool
        :rtype: bool
        """
        if _version_is_older(self.common, other.common):
            return True
        if _version_is_older(self.filters, other.filters):
            return True
        if check_history and _version_is_older(self.history, other.history):
            return True
        for service, version in usix.iteritems(self.service_versions):
            if _version_is_older(version, other.service_versions.get(service)):
                return True
        return False

    @staticmethod
    def get_newest(list_of_versions):
        """
        :type list_of_versions: list[SqueezeVersions] | iter[SqueezeVersions]
        :rtype: SqueezeVersions
        """
        result = SqueezeVersions({}, None, None)
        for versions in list_of_versions:
            if _version_is_older(result.common, versions.common):
                result.common = versions.common
            if _version_is_older(result.history, versions.history):
                result.history = versions.history
            if _version_is_older(result.filters, versions.filters):
                result.filters = versions.filters
            for service, version in usix.iteritems(versions.service_versions):
                if _version_is_older(result.service_versions.get(service), version):
                    result.service_versions[service] = version
        return result

    @staticmethod
    def get_oldest(list_of_versions):
        """
        :type list_of_versions: list[SqueezeVersions] | iter[SqueezeVersions]
        :rtype: SqueezeVersions
        """
        if not list_of_versions:
            return SqueezeVersions({}, None, None)
        result = list(list_of_versions).pop()
        for versions in list_of_versions:
            if versions.common and _version_is_older(versions.common, result.common):
                result.common = versions.common
            if versions.history and _version_is_older(versions.history, result.history):
                result.history = versions.history
            if versions.filters and _version_is_older(versions.filters, result.filters):
                result.filters = versions.filters
            for service, version in usix.iteritems(versions.service_versions):
                if _version_is_older(version, result.service_versions.get(service)):
                    result.service_versions[service] = version
        return result


def get_python_version():
    return ".".join(map(str, sys.version_info[:3]))


def get_revision():
    try:
        return int(open("revision.txt").read().strip())
    except Exception:
        return None


def _version_is_older(version1, version2):
    if version2 is None:
        return False
    if version1 is None:
        return True
    return version1 < version2


SQUEEZE_COMMON_FIELD_NAME = "_common"
SQUEEZE_HISTORY_FIELD_NAME = "_history"
SQUEEZE_FILTERS_FIELD_NAME = "_filters"
SQUEEZE_PYTHON_FIELD_NAME = "_python"
SQUEEZE_REVISION_FIELD_NAME = "_revision"
SQUEEZE_WORKFLOW_URL_FIELD_NAME = "_workflow_url"
SQUEEZE_WAY_FIELD_NAME = "_squeeze_way"

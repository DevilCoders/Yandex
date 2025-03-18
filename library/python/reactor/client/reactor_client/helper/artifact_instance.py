import six

from reactor_client import reactor_objects as r_objs


class ArtifactInstanceBuilder(object):
    def __init__(self):
        self._artifact_id = None
        self._artifact_path = None

        self._value = None

        self._user_time = None

        self._attributes = None

    def set_artifact_id(self, artifact_id):
        """
        :type artifact_id: int
        :rtype: ArtifactInstanceBuilder
        """
        self._artifact_id = artifact_id
        self._artifact_path = None
        return self

    def set_artifact_path(self, path):
        """
        :type path: str
        :rtype: ArtifactInstanceBuilder
        """
        self._artifact_id = None
        self._artifact_path = path
        return self

    @property
    def artifact_identifier(self):
        if self._artifact_id is not None:
            return r_objs.ArtifactIdentifier(artifact_id=self._artifact_id)
        elif self._artifact_path is not None:
            return r_objs.ArtifactIdentifier(namespace_identifier=r_objs.NamespaceIdentifier(namespace_path=self._artifact_path))
        else:
            raise RuntimeError("Artifact is not specified (define either artifact ID or absolute path)!")

    def set_event(self):
        """
        :rtype: ArtifactInstanceBuilder
        """
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.EventArtifactValueProto", dict_obj={})
        return self

    def set_bool(self, value):
        """
        :type value: bool
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(value, bool):
            raise RuntimeError("Invalid value '{}': expected bool, got {}".format(value, type(value)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.BoolArtifactValueProto", dict_obj={'value': value})
        return self

    def set_int(self, value):
        """
        :type value: int
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(value, six.integer_types):
            raise RuntimeError("Invalid value '{}': expected integer, got {}".format(value, type(value)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.IntArtifactValueProto", dict_obj={'value': str(value)})
        return self

    def set_float(self, value):
        """
        :type value: float
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(value, float):
            raise RuntimeError("Invalid value '{}': expected float, got {}".format(value, type(value)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.FloatArtifactValueProto", dict_obj={'value': str(value)})
        return self

    def set_string(self, value):
        """
        :type value: str
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(value, six.string_types):
            raise RuntimeError("Invalid value '{}': expected string, got {}".format(value, type(value)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.StringArtifactValueProto", dict_obj={'value': value})
        return self

    def set_yt_path(self, cluster, path):
        """
        :type cluster: str
        :type path: str
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(cluster, six.string_types):
            raise RuntimeError("Invalid cluster value '{}': expected string, got {}".format(cluster, type(cluster)))
        if not isinstance(path, six.string_types):
            raise RuntimeError("Invalid path value '{}': expected string, got {}".format(path, type(path)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.YtPathArtifactValueProto", dict_obj={'cluster': cluster, 'path': path})
        return self

    def set_sandbox_resource(self, _id, _type=None):
        """
        :type _id: int
        :type _type: str | None
        :rtype: ArtifactInstanceBuilder
        """
        try:
            value = {'id': int(_id)}
        except ValueError:
            raise RuntimeError("Unknown resource id value '{}': expected int, got {}".format(_id, type(_id)))
        if _type and isinstance(_type, six.string_types):
            value['type'] = _type
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.SandboxResourceArtifactValueProto", dict_obj=value)
        return self

    def set_list_bool(self, values):
        """
        :type values: list[bool]
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(values, list) or not all(isinstance(value, bool) for value in values):
            raise RuntimeError("Invalid value '{}': expected list of bools, got {}".format(values, type(values)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.BoolListArtifactValueProto", dict_obj={'values': values})
        return self

    def set_list_int(self, values):
        """
        :type values: list[int]
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(values, list) or not all(isinstance(value, six.integer_types) for value in values):
            raise RuntimeError("Invalid value '{}': expected list of integers, got {}".format(values, type(values)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.IntListArtifactValueProto", dict_obj={'values': [str(value) for value in values]})
        return self

    def set_list_float(self, values):
        """
        :type values: list[float]
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(values, list) or not all(isinstance(value, float) for value in values):
            raise RuntimeError("Invalid value '{}': expected list of floats, got {}".format(values, type(values)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.FloatListArtifactValueProto", dict_obj={'values': [str(value) for value in values]})
        return self

    def set_list_string(self, values):
        """
        :type values: list[str]
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(values, list) or not all(isinstance(value, six.string_types) for value in values):
            raise RuntimeError("Invalid value '{}': expected list of strings, got {}".format(values, type(values)))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.StringListArtifactValueProto", dict_obj={'values': [str(value) for value in values]})
        return self

    def set_list_yt_path(self, values):
        """
        :type values: list[dict[str, str]]
        :rtype: ArtifactInstanceBuilder
        """
        if not isinstance(values, list):
            raise RuntimeError("Invalid value type: expected list of YT paths, got {}".format(type(values)))
        if not all(isinstance(value, dict) and len(value) == 2 and 'cluster' in value and 'path' in value for value in values):
            invalid_values = [value for value in values if not(isinstance(value, dict) and len(value) == 2 and 'cluster' in value and 'path' in value)]
            raise RuntimeError("Invalid value: expected list of YT paths, invalid list items: {}".format(invalid_values))
        self._value = r_objs.Metadata(type_="/yandex.reactor.artifact.YtPathListArtifactValueProto", dict_obj={'values': values})
        return self

    @property
    def value(self):
        return self._value

    def set_user_time(self, user_time):
        """
        :type user_time: datetime
        :rtype: ArtifactInstanceBuilder
        """
        self._user_time = user_time
        return self

    @property
    def user_time(self):
        return self._user_time

    def set_attributes(self, attributes):
        """
        :type attributes: dict[str, str]
        :rtype: ArtifactInstanceBuilder
        """
        self._attributes = attributes
        return self

    @property
    def attributes(self):
        return r_objs.Attributes(self._attributes)

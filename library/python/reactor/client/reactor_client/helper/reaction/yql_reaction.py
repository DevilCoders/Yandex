from .abstract_dynamic_reaction import AbstractDynamicReactionBuilder
from reactor_client import reactor_objects as r_objs


class YqlReactionBuilder(AbstractDynamicReactionBuilder):
    def __init__(self):
        super(YqlReactionBuilder, self).__init__()
        super(YqlReactionBuilder, self).set_reaction_type("yql_operations", "yql_run_exists_query", "1")

        self._access_secret = None
        self._query_id = None
        self._query_title = None

    def set_access_secret(self, nirvana_secret_name):
        """
        :type nirvana_secret_name: str
        :rtype: YqlReactionBuilder
        """
        self._access_secret = nirvana_secret_name
        return self

    def set_query_id(self, yql_query_id):
        """
        :type yql_query_id: str
        :rtype: YqlReactionBuilder
        """
        self._query_id = yql_query_id
        return self

    def set_query_title(self, yql_query_title):
        """
        :type yql_query_title: str
        :rtype: YqlReactionBuilder
        """
        self._query_title = yql_query_title
        return self

    @property
    def parameters_values(self):
        """
        :rtype: dict[str, r_objs.ParametersValueElement]
        """
        parameters_dict = super(YqlReactionBuilder, self).parameters_values

        if self._query_id is None:
            raise RuntimeError("YQL query id is not specified!")
        parameters_dict["queryId"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._query_id)))

        if self._access_secret is not None:
            parameters_dict["accessSecret"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._access_secret)))

        if self._query_title is not None:
            parameters_dict["title"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._query_title)))

        return parameters_dict

    @property
    def inputs_values(self):
        """
        :rtype: dict[str, r_objs.InputsValueElement]
        """
        return {"yqlParametersInputs": r_objs.InputsValueElement(node=r_objs.InputsValueNode(self._dict_to_input_elements_dict(self._inputs)))}

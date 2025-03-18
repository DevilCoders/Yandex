from reactor_client.helper.reaction.sandbox_reaction import SandboxReactionBuilder
from reactor_client import reactor_objects as r_objs


class SandboxDependencyResolverBuilder(SandboxReactionBuilder):
    def __init__(self):
        super(SandboxDependencyResolverBuilder, self).__init__()
        super(SandboxDependencyResolverBuilder, self).set_reaction_type("sandbox_operations", "resolve_dependencies", "1")

        self._output_name = None
        self._demand_ttl = None

    def set_dependencies_output_name(self, output_name):
        """
        :type output_name: str
        :rtype: SandboxDependencyResolverBuilder
        """
        self._output_name = output_name
        return self

    def set_demand_ttl_days(self, demand_ttl=None):
        """
        :type demand_ttl: int | None
        :rtype: SandboxDependencyResolverBuilder
        """
        self._demand_ttl = demand_ttl
        return self

    @property
    def parameters_values(self):
        """
        :rtype: dict[str, r_objs.ParametersValueElement]
        """
        parameters_dict = super(SandboxDependencyResolverBuilder, self).parameters_values

        if self._output_name is None:
            raise RuntimeError("Sandbox task output name with dependencies list is not specified")
        parameters_dict["outputName"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._output_name)))

        if self._demand_ttl is not None:
            parameters_dict["demandsTtl"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._demand_ttl)))
        return parameters_dict

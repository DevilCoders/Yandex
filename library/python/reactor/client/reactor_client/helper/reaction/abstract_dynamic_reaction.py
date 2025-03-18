import six
from abc import abstractmethod

from .abstract_reaction import AbstractReactionBuilder
from reactor_client import reactor_objects as r_objs


class AbstractDynamicReactionBuilder(AbstractReactionBuilder):
    class AbstractRetryPolicyBuilder(object):
        def __init__(self, retry_number):
            """
            :type retry_number: int
            """
            self._retry_number = retry_number

        @property
        @abstractmethod
        def retry_policy_kind(self):
            """
            :rtype: dict[str, r_objs.ParametersValueElement]
            """
            pass

        @property
        def descriptor(self):
            """
            :rtype: r_objs.ParametersValueElement
            """
            retry_policy_dict = self.retry_policy_kind
            retry_policy_dict["retryNumber"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=AbstractDynamicReactionBuilder._make_metadata_from_val(self._retry_number)))
            return r_objs.ParametersValueElement(node=r_objs.ParametersValueNode(retry_policy_dict))

    class UniformRetryPolicyBuilder(AbstractRetryPolicyBuilder):
        def __init__(self, retry_number, delay_millis):
            """
            :type delay_millis: int
            """
            super(AbstractDynamicReactionBuilder.UniformRetryPolicyBuilder, self).__init__(retry_number)
            self._delay_millis = delay_millis

        @property
        def retry_policy_kind(self):
            """
            :rtype: dict[str, r_objs.ParametersValueElement]
            """
            return {
                "uniformRetry": r_objs.ParametersValueElement(node=r_objs.ParametersValueNode({
                    "delay": r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=AbstractDynamicReactionBuilder._make_metadata_from_val(self._delay_millis))),
                })),
            }

    class RandomRetryPolicyBuilder(AbstractRetryPolicyBuilder):
        def __init__(self, retry_number, min_delay_millis, variance_millis):
            """
            :type min_delay_millis: int
            :type variance_millis: int
            """
            super(AbstractDynamicReactionBuilder.RandomRetryPolicyBuilder, self).__init__(retry_number)
            self._min_delay_millis = min_delay_millis
            self._variance_millis = variance_millis

        @property
        def retry_policy_kind(self):
            """
            :rtype: dict[str, r_objs.ParametersValueElement]
            """
            return {
                "randomRetry": r_objs.ParametersValueElement(node=r_objs.ParametersValueNode({
                    "minDelay": r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=AbstractDynamicReactionBuilder._make_metadata_from_val(self._min_delay_millis))),
                    "delayVariance": r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=AbstractDynamicReactionBuilder._make_metadata_from_val(self._variance_millis))),
                })),
            }

    class ExponentialRetryPolicyBuilder(AbstractRetryPolicyBuilder):
        def __init__(self, retry_number, total_time_millis):
            """
            :type total_time_millis: int
            """
            super(AbstractDynamicReactionBuilder.ExponentialRetryPolicyBuilder, self).__init__(retry_number)
            self._total_time_millis = total_time_millis

        @property
        def retry_policy_kind(self):
            """
            :rtype: dict[str, r_objs.ParametersValueElement]
            """
            return {
                "exponentialRetry": r_objs.ParametersValueElement(node=r_objs.ParametersValueNode({
                    "totalTime": r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=AbstractDynamicReactionBuilder._make_metadata_from_val(self._total_time_millis))),
                })),
            }

    def __init__(self):
        super(AbstractDynamicReactionBuilder, self).__init__()
        self._owner = None
        self._retry_policy = None

        self._inputs = {}
        self._outputs = {}

    def set_owner(self, owner):
        """
        :type owner : str
        :rtype: AbstractDynamicReactionBuilder
        """
        self._owner = owner
        return self

    def set_uniform_retries(self, retry_number, delay_millis):
        """
        :type retry_number: int
        :type delay_millis: int
        :rtype: AbstractDynamicReactionBuilder
        """
        self._retry_policy = AbstractDynamicReactionBuilder.UniformRetryPolicyBuilder(retry_number, delay_millis).descriptor
        return self

    def set_random_retries(self, retry_number, min_delay_millis, variance_millis):
        """
        :type retry_number: int
        :type min_delay_millis: int
        :type variance_millis: int
        :rtype: AbstractDynamicReactionBuilder
        """
        self._retry_policy = AbstractDynamicReactionBuilder.RandomRetryPolicyBuilder(retry_number, min_delay_millis, variance_millis).descriptor
        return self

    def set_exponential_retries(self, retry_number, total_time_millis):
        """
        :type retry_number: int
        :type total_time_millis: int
        :rtype: AbstractDynamicReactionBuilder
        """
        self._retry_policy = AbstractDynamicReactionBuilder.ExponentialRetryPolicyBuilder(retry_number, total_time_millis).descriptor
        return self

    def add_input(self, input_name, artifact_reference=None, const=None, expression_var=None):
        """
        :type input_name: str
        :type artifact_reference: r_objs.ArtifactReference | None
        :param const
        :type expression_var: str | None
        :rtype: SandboxReactionBuilder
        """
        if artifact_reference is not None:
            self._inputs[input_name] = artifact_reference
        elif const is not None:
            self._inputs[input_name] = self._make_metadata_from_val(const)
        elif expression_var is not None:
            self._inputs[input_name] = r_objs.ExpressionVariable(expression_var)
        else:
            raise RuntimeError("No input source specified!")
        return self

    def add_output(self, output_name, artifact_reference=None, expression_var=None):
        """
        :type output_name: str
        :type artifact_reference: r_objs.ArtifactReference | None
        :type expression_var: r_objs.ExpressionVariable | None
        :rtype: SandboxReactionBuilder
        """
        if artifact_reference is not None:
            self._outputs[output_name] = artifact_reference
        elif expression_var is not None:
            self._outputs[output_name] = r_objs.ExpressionVariable(expression_var)
        else:
            raise RuntimeError("No output source specified!")
        return self

    @property
    @abstractmethod
    def parameters_values(self):
        parameters_dict = {}

        if self._owner is not None:
            parameters_dict["owner"] = r_objs.ParametersValueElement(value=r_objs.ParametersValueLeaf(generic_value=self._make_metadata_from_val(self._owner)))

        if self._retry_policy is not None:
            parameters_dict["retryPolicy"] = self._retry_policy
        return parameters_dict

    @property
    def inputs_values(self):
        """
        :rtype: dict[str, r_objs.InputsValueElement]
        """
        return self._dict_to_input_elements_dict(self._inputs)

    @property
    def outputs_value(self):
        """
        :rtype: dict[str, r_objs.OutputsValueElement]
        """
        return self._dict_to_output_elements_dict(self._outputs)

    @staticmethod
    def _make_metadata_from_val(val):
        """
        :param val:
        :rtype: r_objs.Metadata
        """
        if isinstance(val, bool):
            proto = "/yandex.reactor.artifact.BoolArtifactValueProto"
            final_val = val
        elif isinstance(val, six.integer_types):
            proto = "/yandex.reactor.artifact.IntArtifactValueProto"
            final_val = str(val)
        elif isinstance(val, six.string_types):
            proto = "/yandex.reactor.artifact.StringArtifactValueProto"
            final_val = str(val)
        else:
            raise RuntimeError("Unknown value type {}".format(type(val)))

        return r_objs.Metadata(
            type_=proto,
            dict_obj={"value": final_val}
        )

    @staticmethod
    def _dict_to_input_elements_dict(dict_):
        """
        :param dict_:
        :rtype: dict[str, r_objs.InputsValueElement]
        """
        inputs_dict = {}
        for k, val in six.iteritems(dict_):
            if isinstance(val, r_objs.ExpressionVariable):
                leaf = r_objs.InputsValueRef(expression_var=val)
            elif isinstance(val, r_objs.Metadata):
                leaf = r_objs.InputsValueRef(const_value=r_objs.InputsValueConst(generic_value=val))
            elif isinstance(val, r_objs.ArtifactReference):
                leaf = r_objs.InputsValueRef(artifact_reference=val)
            else:
                raise RuntimeError("Unknown input value kind " + val + " for input " + k)
            inputs_dict[k] = r_objs.InputsValueElement(value=leaf)
        return inputs_dict

    @staticmethod
    def _dict_to_output_elements_dict(dict_):
        """
        :param dict_:
        :rtype: dict[str, r_objs.OutputsValueElement]
        """
        outputs_dict = {}
        for k, val in six.iteritems(dict_):
            if isinstance(val, r_objs.ExpressionVariable):
                leaf = r_objs.OutputsValueRef(expression_var=val)
            elif isinstance(val, r_objs.ArtifactReference):
                leaf = r_objs.OutputsValueRef(artifact_reference=val)
            else:
                raise RuntimeError("Unknown output value kind " + val + " fro output " + k)
            outputs_dict[k] = r_objs.OutputsValueElement(value=leaf)
        return outputs_dict

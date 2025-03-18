from .utils import convert_type, BaseSerializable


class BaseInput(BaseSerializable):
    def get_name(self):
        raise NotImplementedError

    def get_type(self):
        raise NotImplementedError


class NirvanaBlockInput(BaseInput):
    def __init__(self, block_local_id, block_parameter, type, name):
        self._block_local_id = block_local_id
        self._block_parameter = block_parameter
        self._name = name
        self._type = type

    def get_name(self):
        return self._name

    def get_type(self):
        return convert_type(self._type)

    def to_dict(self):
        return {
            "parameter": "{block_local_id}; {block_parameter}; ${{{name}}}".format(
                block_local_id=self._block_local_id,
                block_parameter=self._block_parameter,
                name=self._name
            ),
            "parameter-type": "block",
            "output-type": self.get_type(),
        }


class NirvanaBlockJsonOutput(BaseSerializable):
    def __init__(self, block_local_id, block_json_output, json_path, output_type, output_name):
        self._block_local_id = block_local_id
        self._block_json_output = block_json_output
        self._json_path = json_path
        self._output_type = output_type
        self._output_name = output_name

    def to_dict(self):
        return {
            "result-type": "value-from-json",
            "template-result": "{block_local_id}; {block_json_output}".format(block_local_id=self._block_local_id, block_json_output=self._block_json_output),
            "result-path": self._json_path,
            "output-type": convert_type(self._output_type),
            "output-name": self._output_name,
        }


class NirvanaFunction(BaseSerializable):
    def __init__(self, workflow_id, inputs, outputs):
        self._workflow_id = workflow_id
        self._inputs = inputs
        self._outputs = outputs

    def to_dict(self):
        input_space = {i.get_name(): i.get_type() for i in self._inputs}
        parameters = [i.to_dict() for i in self._inputs]
        outputs = [o.to_dict() for o in self._outputs]
        return {
            "function-type": "nirvana-function",
            "nirvana-function-template": {
                "workflow-id": self._workflow_id,
                "input-space": input_space,
                "parameters": parameters,
                "outputs": outputs,
            }
        }


class ExistingFunction(BaseSerializable):
    def __init__(self, function_id):
        self._function_id = function_id

    def to_dict(self):
        return {
            "function-type": "existing-function",
            "function-id": self._function_id,
        }

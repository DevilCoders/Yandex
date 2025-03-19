import json
import operator
import re

from yc_common.models import jsonschema_for_model


class SimpleSwaggerWriter:
    def __init__(self, basepath, version, description):
        self.basepath = basepath
        self.version = version
        self.description = description
        self._paths = {}
        self._objects_definitions = {}
        self._tags = {}

    # :schema - jsl object; default serialization format is incompatible with swagger
    def add_api_request(
        self, method, path, handler,
        params_model, request_model, response_model, module_name, module_description
    ):
        if isinstance(path, (list, tuple)):
            path = path[0]

        # Adding ? after the qualifier makes it perform the match in non-greedy or minimal fashion
        path = path.replace("<", "{").replace(">", "}")

        pos = module_name.rfind(".")
        if pos > -1:
            module_name = module_name[pos + 1:]

        path_spec = self._paths.setdefault(path, {})
        method_spec = path_spec.setdefault(method.lower(), {"parameters": [], "tags": [], "responses": {}})
        self._tags.setdefault(module_name, module_description)

        method_spec["tags"].append(module_name)
        method_spec["description"] = "" if handler.__doc__ is None else handler.__doc__
        method_spec["summary"] = method_spec["description"]
        method_spec["operationId"] = handler.__name__

        params_list = method_spec["parameters"]
        responses_spec = method_spec["responses"]

        for path_param in re.findall("{(.+?)}", path):
            params_list.append({
                "name": path_param,
                "in": "path",
                "type": "string",
                "required": True
            })

        if params_model is not None:
            query_def = jsonschema_for_model(params_model)
        else:
            query_def = None

        if query_def is not None:
            for query_name, query_value in query_def["properties"].items():
                query_arg = {
                    "name": query_name,
                    "in": "query",
                    "required": query_name in query_def.get("required", [])
                }
                query_arg.update(query_value)
                params_list.append(query_arg)

        if request_model is not None:
            request_schema = jsonschema_for_model(request_model)
            name, object_def = request_schema['title'], request_schema
            self.__add_object_definition(name, object_def)
            params_list.append({
                "name": "body",
                "in": "body",
                "schema": {"$ref": "#/definitions/" + name}
            })

        if response_model is not None:
            response_schema = jsonschema_for_model(response_model, public=True)
            name, object_def = response_schema["title"], response_schema
            name += "Model"  # Suffix to avoid conflict between responses and models
            self.__add_object_definition(name, object_def)
            responses_spec["200"] = {
                "schema": {"$ref": "#/definitions/" + name},
                "description": response_model.__doc__ or ""
            }
        else:
            responses_spec["200"] = {"description": "OK"}

    def __add_object_definition(self, name, object_def):
        if name not in self._objects_definitions:
            self._objects_definitions[name] = object_def
        elif self._objects_definitions[name] != object_def:
            raise UserWarning(
                'Found different structures with the same name '
                'while generating documentation. Name: ' + name
            )

    def get_json_doc(self):
        specs = {
            "swagger": "2.0",
            "basePath": self.basepath,
            "produces": ["application/json"],
            "paths": self._paths,
            "info": {"version": self.version, "title": self.description},
            "definitions": self._objects_definitions,
            "tags": [{
                "name": tag,
                "description": tag_description,
            } for tag, tag_description in sorted(self._tags.items(), key=operator.itemgetter(0))]
        }

        return json.dumps(specs, indent=4)

import json


class DcMeta:
    def __init__(self, json):
        self._process_id = json["processId"]
        self._owner = json["owner"]
        self._description = json.get("description", None)

    def get_process_id(self):
        return self._process_id

    def get_owner(self):
        return self._owner

    def get_description(self):
        return self._description


class DcReferencedEntity:
    def __init__(self, json):
        self._id = json["id"]
        self._entity_type = json["entityType"]
        self._domain = json["domain"]
        self._name = json["name"]
        self._description = json.get("description", None)
        self._data = json["data"]

    def get_id(self):
        return self._id

    def get_entity_type(self):
        return self._entity_type

    def get_domain(self):
        return self._domain

    def get_name(self):
        return self._name

    def get_description(self):
        return self._description

    def get_data(self):
        return self._data


class DcContext:
    def __init__(self, json):
        self._meta = DcMeta(json["meta"])
        self._parameters = json.get("parameters", {})
        self._entities = {e["id"]: DcReferencedEntity(e) for e in json['entities']}

    def get_meta(self):
        return self._meta

    def get_parameters(self):
        return self._parameters

    def get_entities(self):
        return self._entities


def context():
    with open('dc_context.json') as context:
        return DcContext(json.load(context))


if __name__ == 'main' and __package__ is None:
    __package__ = 'nirvana.dc_context'

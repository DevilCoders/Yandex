import jsonobject


class HashableJsonObject(jsonobject.JsonObject):
    __FROZENSET_HASHED_PROPERTIES = (
        jsonobject.SetProperty,
        jsonobject.ListProperty,
        jsonobject.DictProperty,
    )

    def __get_hashable_property_value(self, property_obj):
        if isinstance(property_obj, self.__FROZENSET_HASHED_PROPERTIES) and self[property_obj.name] is not None:
            return frozenset(self[property_obj.name])
        return self[property_obj.name]

    def __hash__(self):
        return hash(tuple(
            self.__get_hashable_property_value(self.properties()[property_key])
            for property_key in sorted(self.properties().keys())
        ))

    def __eq__(self, other):
        return all(
            getattr(self, property, None) == getattr(other, property, None)
            for property in self.properties()
        )

    def __ne__(self, other):
        return not self.__eq__(other)

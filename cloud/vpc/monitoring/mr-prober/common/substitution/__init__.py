import re
from typing import Dict, Any

VARIABLE_PATTERN: re.Pattern = re.compile(r"\${\s*(?P<key>[A-Za-z0-9._-]+)\s*}")
SKIP_VARIABLE_PATTERN: re.Pattern = re.compile(r"\${{(?P<value>.+?)}}")
COMPOSITE_KEY_SPLITTER = "."
TYPES_ALLOWED_FOR_PARTIAL_SUBSTITUTION = str, int, float

# the limit of the variables count in the line
VARIABLES_COUNT_LIMIT = 12


def get_value_of_composite_key(composite_key: str, variables_map: Dict[str, Any]) -> Any:
    keys = composite_key.split(COMPOSITE_KEY_SPLITTER)
    for i, key in enumerate(keys):
        if not isinstance(variables_map, dict):
            raise ValueError(
                f"key '{COMPOSITE_KEY_SPLITTER.join(keys[:i + 1])}' assumes than "
                f"value of '{COMPOSITE_KEY_SPLITTER.join(keys[:i])}' must be 'dict', "
                f"but found '{type(variables_map).__name__}'"
            )
        if key == "":
            raise ErrorEmptyKey(composite_key, len(COMPOSITE_KEY_SPLITTER.join(keys[:i])))
        try:
            variables_map = variables_map[key]
        except KeyError:
            raise KeyError(COMPOSITE_KEY_SPLITTER.join(keys[:i + 1]))
    return variables_map


def substitute(value: str, variables_map: Dict[str, Any], depth: int = 0) -> Any:
    """
    This function allows you to substitute any phrase with a given pattern with value
    found in the variable map by the key. Nested variables are supported.

    If only part of the string is replaced in value,
    then the substituted value must have the simple type else an exception will be raised.
    If value fully fits the specified pattern, then the new value will be substituted
    instead of the old one. Thus type substitution is possible.
    """

    for i in range(depth, VARIABLES_COUNT_LIMIT):
        match = VARIABLE_PATTERN.search(value)
        if not match:
            # all variables are substituted
            return SKIP_VARIABLE_PATTERN.sub(lambda match_obj: "${" + f"{match_obj.group('value')}" + "}", value)
        try:
            new_value = get_value_of_composite_key(match.group("key"), variables_map)
        except KeyError as ex:
            # to save the result of previous substitutions when KeyError, we use a custom exception
            raise SubstituteKeyError(value=value, key=ex.args[0], depth=i)
        if match.span() == (0, len(value)):
            # in this case, the new value completely replaces the old one
            # NOTE: it allows to change the type
            return new_value
        if not isinstance(new_value, TYPES_ALLOWED_FOR_PARTIAL_SUBSTITUTION):
            raise TypeError(
                f"when substituting part of a string, only the following types can be used: "
                f"{', '.join(x.__name__ for x in TYPES_ALLOWED_FOR_PARTIAL_SUBSTITUTION)}"
            )
        value = value[:match.start()] + str(new_value) + value[match.end():]
    raise Exception(f"the limit of the variables count has been reached: max {VARIABLES_COUNT_LIMIT}")


class ErrorEmptyKey(Exception):
    def __init__(self, value: str, position: int):
        super().__init__(f"empty key after position {position} in value '{value}'")


class SubstituteKeyError(KeyError):
    def __init__(self, value: str, key: str, depth: int = 0):
        self.depth = depth
        self.value = value
        self.key = key
        super().__init__(self.key)

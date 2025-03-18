from .__guid import create, to_string, parse  # noqa


def guid():
    return to_string(create())

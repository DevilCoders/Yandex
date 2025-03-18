import pytest

unstable_only = pytest.mark.unstable_only


def _has_group(api, group):
    result = api.api.get("/unstable/fastconf/forza/groups")
    return group in result["groups"]


@unstable_only
def test_fastconf(api):
    GROUP_NAME = "MSK_UGRB_TESTING_GROUP_BASE"

    assert (not _has_group(api, GROUP_NAME))

    # add group
    add_data = {
        "action": "add",
        "group": GROUP_NAME,
        "owners": ["test_owner1", "test_owner2"],
        "description": "my sample group",
        "ctype": "test",
        "itype": "base",
        "prj": ["prj1", "prj2", "prj3"],
        "min_power": 1000,
        "min_replicas": 4,
        "memory": "12 Gb",
        "disk": "100 Gb",
        "location": "MSK_UGRB"
    }
    api.api.post("/unstable/fastconf/forza/groups", add_data)
    assert (_has_group(api, GROUP_NAME))

    # get instances
    api.api.get("/unstable/groups/%s/instances" % GROUP_NAME)
    api.api.get("/unstable/searcherlookup/groups/%s/instances" % GROUP_NAME)

    # remove group
    remove_data = {
        "action": "remove",
        "group": GROUP_NAME
    }
    api.api.post("/unstable/fastconf/forza/groups", remove_data)
    assert (not _has_group(api, GROUP_NAME))

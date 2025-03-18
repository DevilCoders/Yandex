import json

from utils.mongo import update_wbeapi_cache


def test_show():
    util_options = {
        "action": update_wbeapi_cache.EActions.SHOW,
        "backend_type": update_wbeapi_cache.EBackendTypes.API,
        "commit": 2402547,
        "request": "/ctypes/experiment",
    }

    result = update_wbeapi_cache.jsmain(util_options)
    result = json.loads(result)

    assert result["name"] == "experiment"
    assert result["descr"] == "Experimental machines (usually temporary group)"

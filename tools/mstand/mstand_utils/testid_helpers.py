def testid_from_adminka(testid: str) -> bool:
    return testid and testid.isdigit() and not testid_is_all(testid)


def testid_is_bro(testid: str) -> bool:
    return testid and testid.startswith("mbro__")


def testid_is_adv(testid: str) -> bool:
    return testid and testid.startswith("a_") and len(testid) > 2 and testid[2:].isdigit()


def convert_adv_testid(testid: str) -> str:
    assert testid
    assert testid.startswith("a_")
    result = testid[2:]
    assert result
    assert result.isdigit()
    return result


def testid_is_simple(testid: str) -> bool:
    return testid and testid.isdigit() and testid != "0"


def testid_is_all(testid: str) -> bool:
    return testid == "all" or testid == "0"

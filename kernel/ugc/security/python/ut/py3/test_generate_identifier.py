from kernel.ugc.security.python import generate_identifier
from pytest import raises


def test_generate_identifier():
    assert generate_identifier("/my/awesome/ugc/", "/user/123456") == "CQ7IK_jWBKgp4rF65kAejuTu6dybzl"


def test_generate_identifier_cyrillic():
    result = generate_identifier("/my/awesome/ugc/", "/user/123456", context_id="текст отзыва")
    assert result == "HjQjAiU-agSV4R1CI3X7OsmpUkWbu6zpY"
    assert isinstance(result, str)


def test_generate_identifier_binary():
    result = generate_identifier(b"/my/awesome/ugc/", b"/user/123456", context_id="текст отзыва")
    assert result == b"HjQjAiU-agSV4R1CI3X7OsmpUkWbu6zpY"
    assert isinstance(result, bytes)


def test_generate_identifier_cyrillic_unicode():
    assert generate_identifier("/my/awesome/ugc/", "/user/123456", context_id="текст отзыва") == "HjQjAiU-agSV4R1CI3X7OsmpUkWbu6zpY"


def test_generate_identifier_context_id():
    assert generate_identifier("/my/awesome/ugc/", "/user/123456", context_id="some identifier") == "pa23nK6lpJWtXDUs_T_mrWOHonSUFC"


def test_generate_identifier_parent_id():
    assert generate_identifier("/my/awesome/ugc/", "/user/123456", parent_id="some identifier") == "FLkrN4n7FoKbhagsNAB-75EgQcY_9pPG"


def test_generate_identifier_mix():
    assert generate_identifier("/my/awesome/ugc/", "/user/123456", context_id="some identifier", client_entropy="entropy") == "NDVOejH7JVrxXlhc3MwvYwd9fDVFbWlBp"


def test_generate_identifier_fails_short_ns():
    with raises(RuntimeError):
        generate_identifier("", "/user/123456")


def test_generate_identifier_fails_short_user_id():
    with raises(RuntimeError):
        generate_identifier("/my/awesome/ugc/", "")

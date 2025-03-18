#!/usr/bin/env python

import auth_client_parser as ap

import pytest
from time import time


def create_cookie(ver, delta, body):
    ts = int(time()) + delta
    return ((str(ver) + ":") if ver else "") + str(ts) + body, ts


def test_broken_oauth_token():

    token = ap.OAuthToken("")
    assert not token.ok

    token = ap.OAuthToken("12312345")
    assert not token.ok

    token = ap.OAuthToken("AAAAAAAAAAAA")
    assert not token.ok

    token = ap.OAuthToken("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA&%")
    assert not token.ok

    token = ap.OAuthToken("1.1000.10.1513005645.1513002045123.111")
    assert not token.ok

    token = ap.OAuthToken("1.4001096104.2210.1513005645.1513002045123.13446.J8T3tLyIJYWfeqwc.V0juP0.WhK8#q")
    assert not token.ok

    token = ap.OAuthToken("AQAAAADu98kGAAAEJZJ-K8Kk9kEghNMjIsfeiNs==")
    assert not token.ok

    token = ap.OAuthToken("0.4001096104.2210.1513005645.1513002045123.13446.J8T3tLyIJYWfeqwc.V0juP0.WhK8kq")
    assert not token.ok

    token = ap.OAuthToken("2.4001096104.2210.1513005645.1513002045123.13446.J8T3tLyIJYWfeqwc.V0juP0.WhK8kq")
    assert not token.ok

    token = ap.OAuthToken("1.40010961O4.2210.1513005645.1513002045123.13446.J8T3tLyIJYWfeqwc.V0juP0.WhK8kq")
    assert not token.ok

    token = ap.OAuthToken("g0_AQAAAAAAAYiUAAAAgQAAAAA63mixeHh4eHh4eHh4eHh4eHh4eMmOnSA")
    assert not token.ok


def test_embedded_oauth_token():

    token = ap.OAuthToken("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
    assert token.ok
    assert token.uid == 0

    token = ap.OAuthToken("AQAAAADu98kGAAAEJZJ-K8Kk9kEghNMjIsfeiNs")
    assert token.ok
    assert token.uid == 4009216262

    token = ap.OAuthToken("y0_AQAAAAAAAYiUAAAAgQAAAAA63mixeHh4eHh4eHh4eHh4eHh4eMmOnSA")
    assert token.ok
    assert token.uid == 100500


def test_stateless_oauth_token():
    token = ap.OAuthToken(
        "1.4001096104.2210.1513005645.1513002045123.13446.J8T3tLyIJYWfeqwc."
        "V0juP0P8-VNcn3mj9Dd2S4iwxJSj_K_j20Wikjx8-K5SzuNrEEYGkXsGWG3xz3_YHo67xFof1t8eHTDZWj"
        "9SjwKV5uQT7RCo1W6cOc0bUAq5oKhDna2Tk7LRWAR4y5xurzvrfgo1-0o8ZNO7cmW3OCK2z1X-orP_BEO_"
        "Q7OppNhaQGMqr5PkxB1vAWltaxSEFIXJjelClUhNEXdZzosRQ7C8GLvLJGYJw8Htf4fE4w.WhK8_q0I-n9k"
    )
    assert token.ok
    assert token.uid == 4001096104


def test_invalid_cookie():

    cookie = ap.Cookie("")
    assert cookie.status == ap.ParseStatus.Invalid

    cookie = ap.Cookie("aa", 100)
    assert cookie.status == ap.ParseStatus.Invalid

    cookie = ap.Cookie("1.4001096104.2210.1513005645.1513002045123.13446.J8T3tLyIJYWfeqwc.V0juP0.WhK8#q")
    assert cookie.status == ap.ParseStatus.Invalid

    cookie = ap.Cookie("noauth")
    assert cookie.status == ap.ParseStatus.Invalid

    cookie = ap.Cookie("5:5.-23.4.*23456764.2:434:244.8:2323:2323.13:12323.0:23453.4343.ab23424cde45646ff45")
    assert cookie.status == ap.ParseStatus.Invalid

    cookie = ap.Cookie(
        "2:12ff323.-23.4.23456764.2:434:244.8:2323:2323.13:12323.1.10.YA_RU:23453.4343.ab23424cde45646ff45"
    )

    assert cookie.status == ap.ParseStatus.Invalid
    with pytest.raises(RuntimeError):
        cookie.session_info()
    with pytest.raises(RuntimeError):
        cookie.default_user()
    with pytest.raises(RuntimeError):
        cookie.users()


def test_noauth_cookie():

    cookie = ap.Cookie("noauth:1000")
    assert cookie.status == ap.ParseStatus.NoauthValid
    assert cookie.session_info().version == 1
    assert cookie.session_info().ts == 1000

    cookie = ap.Cookie("noauth:1000", 1400000000)
    assert cookie.status == ap.ParseStatus.NoauthValid
    assert cookie.session_info().version == 1
    assert cookie.session_info().ts == 1000


def test_v1_cookie():

    cookie = ap.Cookie(
        "1328973366.0.1.38380791.2:100173605:230.8:1328973366892:1845265753:50.81964.90501.8c84c2f8db3963b5d48c56",
        1515151515.0,
    )
    assert cookie.status == ap.ParseStatus.RegularExpired
    assert cookie.session_info().version == 1
    assert cookie.session_info().ttl == 1
    assert cookie.session_info().ts == 1328973366
    assert cookie.session_info().auth_id == "1328973366892:1845265753:50"
    assert not cookie.session_info().safe
    assert not cookie.session_info().suspicious
    assert not cookie.session_info().stress
    assert cookie.session_info().ext_attrs == {}
    assert cookie.default_user().uid == 38380791
    assert cookie.default_user().pwd_check_delta == 0
    assert cookie.default_user().lang == 1
    assert cookie.default_user().social_id == 0
    assert not cookie.default_user().lite
    assert cookie.default_user().have_pwd
    assert not cookie.default_user().staff
    assert not cookie.default_user().betatester
    assert not cookie.default_user().glogouted
    assert not cookie.default_user().partner_pdd_token
    assert not cookie.default_user().secure
    assert cookie.default_user().ext_attrs == {}
    assert len(cookie.users()) == 1

    cookie = ap.Cookie(
        "1328973366.0.1.38380791.2:100173605:230.8:1328973366892:1845265753:50.81964.90501.8c84c2f8db3963b5d48c56",
        1328973400,
    )
    assert cookie.status == ap.ParseStatus.RegularMayBeValid
    assert cookie.session_info().version == 1
    assert cookie.session_info().ttl == 1
    assert cookie.session_info().ts == 1328973366
    assert cookie.session_info().auth_id == "1328973366892:1845265753:50"
    assert not cookie.session_info().safe
    assert not cookie.session_info().suspicious
    assert not cookie.session_info().stress
    assert cookie.session_info().ext_attrs == {}
    assert cookie.default_user().uid == 38380791
    assert cookie.default_user().pwd_check_delta == 0
    assert cookie.default_user().lang == 1
    assert cookie.default_user().social_id == 0
    assert not cookie.default_user().lite
    assert cookie.default_user().have_pwd
    assert not cookie.default_user().staff
    assert not cookie.default_user().betatester
    assert not cookie.default_user().glogouted
    assert not cookie.default_user().partner_pdd_token
    assert not cookie.default_user().secure
    assert cookie.default_user().ext_attrs == {}
    assert len(cookie.users()) == 1


def test_v2_cookie():

    cookie = ap.Cookie(
        "2:1365499355.-834.0.201238231.8:1365499355084:1406987487:49.669:1.1.8.1.-1.92087.1638.c8a1c25e0922c524f38d19"
    )
    assert cookie.status == ap.ParseStatus.RegularExpired
    assert cookie.session_info().version == 2
    assert cookie.session_info().ttl == 0
    assert cookie.session_info().ts == 1365499355
    assert cookie.session_info().auth_id == "1365499355084:1406987487:49"
    assert cookie.session_info().safe
    assert not cookie.session_info().suspicious
    assert not cookie.session_info().stress
    assert cookie.session_info().ext_attrs == {}
    assert cookie.default_user().uid == 201238231
    assert cookie.default_user().pwd_check_delta == -1
    assert cookie.default_user().lang == 8
    assert cookie.default_user().social_id == 0
    assert not cookie.default_user().lite
    assert cookie.default_user().have_pwd
    assert cookie.default_user().staff
    assert not cookie.default_user().betatester
    assert not cookie.default_user().glogouted
    assert not cookie.default_user().partner_pdd_token
    assert not cookie.default_user().secure
    assert cookie.default_user().ext_attrs == {}
    assert len(cookie.users()) == 1

    cookie = ap.Cookie(
        "2:1365499355.-834.0.201238231.8:1365499355084:1406987487:49.669:1.1.8.1.-1.92087.1638.c8a1c25e0922c524f38d19",
        1365499885,
    )
    assert cookie.status == ap.ParseStatus.RegularMayBeValid
    assert cookie.session_info().version == 2
    assert cookie.session_info().ttl == 0
    assert cookie.session_info().ts == 1365499355
    assert cookie.session_info().auth_id == "1365499355084:1406987487:49"
    assert cookie.session_info().safe
    assert not cookie.session_info().suspicious
    assert not cookie.session_info().stress
    assert cookie.session_info().ext_attrs == {}
    assert cookie.default_user().uid == 201238231
    assert cookie.default_user().pwd_check_delta == -1
    assert cookie.default_user().lang == 8
    assert cookie.default_user().social_id == 0
    assert not cookie.default_user().lite
    assert cookie.default_user().have_pwd
    assert cookie.default_user().staff
    assert not cookie.default_user().betatester
    assert not cookie.default_user().glogouted
    assert not cookie.default_user().partner_pdd_token
    assert not cookie.default_user().secure
    assert cookie.default_user().ext_attrs == {}
    assert len(cookie.users()) == 1


def test_v3_cookie():

    c, ts = create_cookie(
        3,
        -100 * 24 * 3600,
        ".5.1.1432729765750:5ArCbQ:4c.102.1:2|1130000013409810.2516064.403.0:7.2:2516064|"
        "243870575.22383952.1302.1:1234567.2:22383952|1130000017410839.26675722.0.2:26675755|174165.377228.Ae9uNnIL5oHl1tT1Q",
    )
    cookie = ap.Cookie(c)
    assert cookie.status == ap.ParseStatus.RegularExpired
    assert cookie.session_info().version == 3
    assert cookie.session_info().ttl == 5
    assert cookie.session_info().ts == ts
    assert cookie.session_info().auth_id == "1432729765750:5ArCbQ:4c"
    assert not cookie.session_info().safe
    assert cookie.session_info().suspicious
    assert cookie.session_info().stress
    assert cookie.session_info().ext_attrs == {1: "2"}
    assert cookie.default_user().uid == 243870575
    assert cookie.default_user().pwd_check_delta == 22383952
    assert cookie.default_user().lang == 1
    assert cookie.default_user().social_id == 1234567
    assert not cookie.default_user().lite
    assert cookie.default_user().have_pwd
    assert cookie.default_user().staff
    assert cookie.default_user().betatester
    assert not cookie.default_user().glogouted
    assert not cookie.default_user().partner_pdd_token
    assert cookie.default_user().secure
    assert cookie.default_user().ext_attrs == {1: "1234567", 2: "22383952"}
    assert len(cookie.users()) == 3
    assert cookie.users()[0].uid == 1130000013409810
    assert cookie.users()[0].pwd_check_delta == 2516064
    assert cookie.users()[0].lang == 7
    assert cookie.users()[0].social_id == 0
    assert cookie.users()[0].lite
    assert cookie.users()[0].have_pwd
    assert not cookie.users()[0].staff
    assert not cookie.users()[0].betatester
    assert cookie.users()[0].glogouted
    assert not cookie.users()[0].partner_pdd_token
    assert not cookie.users()[0].secure
    assert cookie.users()[0].ext_attrs == {0: "7", 2: "2516064"}
    assert cookie.users()[1].uid == 243870575
    assert cookie.users()[1].pwd_check_delta == 22383952
    assert cookie.users()[1].lang == 1
    assert cookie.users()[1].social_id == 1234567
    assert not cookie.users()[1].lite
    assert cookie.users()[1].have_pwd
    assert cookie.users()[1].staff
    assert cookie.users()[1].betatester
    assert not cookie.users()[1].glogouted
    assert not cookie.users()[1].partner_pdd_token
    assert cookie.users()[1].secure
    assert cookie.users()[1].ext_attrs == {1: "1234567", 2: "22383952"}
    assert cookie.users()[2].uid == 1130000017410839
    assert cookie.users()[2].pwd_check_delta == 26675722
    assert cookie.users()[2].lang == 1
    assert cookie.users()[2].social_id == 0
    assert not cookie.users()[2].lite
    assert not cookie.users()[2].have_pwd
    assert not cookie.users()[2].staff
    assert not cookie.users()[2].betatester
    assert not cookie.users()[2].glogouted
    assert not cookie.users()[2].partner_pdd_token
    assert not cookie.users()[2].secure
    assert cookie.users()[2].ext_attrs == {2: "26675755"}

    c, ts = create_cookie(
        3,
        -10 * 24 * 3600,
        ".5.1.1432729765750:5ArCbQ:4c.102.1:2|1130000013409810.2516064.403.0:7.2:2516064|"
        "243870575.22383952.1302.1:1234567.2:22383952|1130000017410839.26675722.0.2:26675755|174165.377228.Ae9uNnIL5oHl1tT1Q",
    )
    cookie = ap.Cookie(c)
    assert cookie.status == ap.ParseStatus.RegularMayBeValid
    assert cookie.session_info().version == 3
    assert cookie.session_info().ttl == 5
    assert cookie.session_info().ts == ts
    assert cookie.session_info().auth_id == "1432729765750:5ArCbQ:4c"
    assert not cookie.session_info().safe
    assert cookie.session_info().suspicious
    assert cookie.session_info().stress
    assert cookie.session_info().ext_attrs == {1: "2"}
    assert cookie.default_user().uid == 243870575
    assert cookie.default_user().pwd_check_delta == 22383952
    assert cookie.default_user().lang == 1
    assert cookie.default_user().social_id == 1234567
    assert not cookie.default_user().lite
    assert cookie.default_user().have_pwd
    assert cookie.default_user().staff
    assert cookie.default_user().betatester
    assert not cookie.default_user().glogouted
    assert not cookie.default_user().partner_pdd_token
    assert cookie.default_user().secure
    assert cookie.default_user().ext_attrs == {1: "1234567", 2: "22383952"}
    assert len(cookie.users()) == 3
    assert cookie.users()[0].uid == 1130000013409810
    assert cookie.users()[0].pwd_check_delta == 2516064
    assert cookie.users()[0].lang == 7
    assert cookie.users()[0].social_id == 0
    assert cookie.users()[0].lite
    assert cookie.users()[0].have_pwd
    assert not cookie.users()[0].staff
    assert not cookie.users()[0].betatester
    assert cookie.users()[0].glogouted
    assert not cookie.users()[0].partner_pdd_token
    assert not cookie.users()[0].secure
    assert cookie.users()[0].ext_attrs == {0: "7", 2: "2516064"}
    assert cookie.users()[1].uid == 243870575
    assert cookie.users()[1].pwd_check_delta == 22383952
    assert cookie.users()[1].lang == 1
    assert cookie.users()[1].social_id == 1234567
    assert not cookie.users()[1].lite
    assert cookie.users()[1].have_pwd
    assert cookie.users()[1].staff
    assert cookie.users()[1].betatester
    assert not cookie.users()[1].glogouted
    assert not cookie.users()[1].partner_pdd_token
    assert cookie.users()[1].secure
    assert cookie.users()[1].ext_attrs == {1: "1234567", 2: "22383952"}
    assert cookie.users()[2].uid == 1130000017410839
    assert cookie.users()[2].pwd_check_delta == 26675722
    assert cookie.users()[2].lang == 1
    assert cookie.users()[2].social_id == 0
    assert not cookie.users()[2].lite
    assert not cookie.users()[2].have_pwd
    assert not cookie.users()[2].staff
    assert not cookie.users()[2].betatester
    assert not cookie.users()[2].glogouted
    assert not cookie.users()[2].partner_pdd_token
    assert not cookie.users()[2].secure
    assert cookie.users()[2].ext_attrs == {2: "26675755"}

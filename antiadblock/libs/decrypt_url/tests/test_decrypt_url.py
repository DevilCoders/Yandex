# coding=utf-8
import pytest

from antiadblock.libs.decrypt_url.lib import decrypt_url, DecryptionError,\
    resplit_using_length, get_key, decrypt_xor, decrypt_base64, is_crypted_url

TEST_SECRET_KEY = "duoYujaikieng9airah4Aexai4yek4qu"


@pytest.mark.parametrize('crypted, expected, prefix, is_trailing_slash_enabled',
                         [
                             ("/XWtN9S102/my20071_/mQbvqmJEo8M/sBeR2_L/la-p_d/UVGqH/s381lrh/Ck9Lh5GGE_/KXLzr/N3DvlbQb/hLESeQU/febTuIvVIYZvTSQ/",
                              "//yastatic.net/partner-code-bundles/3962/context_static_adb.js", "/", True),
                             ("/a2ZMj3K56/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNdJ0X9DEw",
                              "//an.yandex.ru/resource/banner.gif", "/", False),
                             ("/a2ZMj6K50/1e8a6f2_m_Kc/__py4D/MbQ-ve2/XEFsSqI/-isFGz_/Sp6lNB/wmHx9ErKKcUxF",
                              "https://awaps.yandex.net/YkZbKgdWgD.png", "/", False),
                             ("/a2ZMj4K54/1e8a6f2_m_KY/bqp3IN/K7Bgobe/GFEdYqZ/jhv12p_/Uwuvd5flH5ABprA/",
                              "http://some.other.domain/??sldkfjsld", "/", True),
                             ("/a9ZMj9K02/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNd/J0X9DE/9bva29/DlH9vmG6O/PJCLU9Q/mEQTowIZf/ujdK/G_KM49THqj/_Sv/8l0BAKGIbSI" +
                              "/uyR/dPISk57pH/vYs9e/L_XcoxaX/TyuzXRD/Z6VDYoBL/niyTkHPoF/xYW/0taWc5Q/QZzfTqN7f/_aMezYD/Lci40pD/uLtYo0ey--ncO4",
                              "//an.yandex.ru/resource/banner.gif", "/", False),
                             ("/a9ZMj9K02/1e8a6f2_m_Kc/__py4D/MbQ-ve2/XEFsSqI/-isFGz_/Sp6lNB/wmHx9E/rKKcUx/F7ntquWO9/MY-TVsc/FIwPV9LJz/gTRK/C--K_P3IuB" +
                              "/f0o/sJgBCivD7ul/qiJ/aDIai2bZ-/joEYd/b3IYrFxW/xC_10NV/a5ZZdZVH/jA2Ci3PXF/SAY/9ciDdJM/OXBvDpNvQ/5Jkzyan/4aSU3kA/29j5wAQDO7k8mQ",
                              "https://awaps.yandex.net/YkZbKgdWgD.png", "/", False),
                             ("/a9ZMj9K02/1e8a6f2_m_KY/bqp3IN/K7Bgobe/GFEdYqZ/jhv12p_/Uwuvd5/flH5AB/prAIWl/Ig1Fau22L/Eom9Sto/uKRb-_Yd_/pw5M/Cc-H2P3FrB/Pdo/-Z7IzOzCp2s/" +
                              "lDp/gI6W9xY1f/o6EJS/IDbSppRW/TeB7VlL/V691a4dg/kzeZrUnBD/hUn/-e-9abc/iYi7iq_nZ/-Kcp1IL/5RCE1mT/y_tqsHUTODrv-0/",
                              "http://some.other.domain/??sldkfjsld", "/", True),
                             ("/test/a2ZMj3K56/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNdJ0X9DEw",
                              "//an.yandex.ru/resource/banner.gif", "/test/", False),
                             ("/test/a2ZMj3K56/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNdJ0X9DEw",
                              "//an.yandex.ru/resource/banner.gif", "/(?:test|prefix)/", False),
                             ("/SNXt17713/my2007kK/Kdf_roahEmJ/YFCQ3yB/y6O_-p/UJCeL/r1dEu7R/eltbtwCT9z/eyWz_/4GNuUzJZ/1-fLq5V/LqKN9OPS/cNR/zETIFcs-yi2/yk5nU/IbXzXA/" +
                              "Rcuydyr6b/zaR1Do7/YmaZKwb/vPu7/cA49RHsp/ms3G_-YKn/HNTgEn2/FHDzyYSE6w.png",
                              "https://storage.mds.yandex.net/get-canvas-html5/895207/7f5026e4-7dbc-4068-9155-9e25a78db93d/images/Tab_S4_728x90_atlas_P_.png", "/", True),
                             ("/XWtN2S889/my20071_/mdavqma1I6M/o9cDWuLgqG_6A/index.html",
                              "//test.local/pogoda/index.html", "/", True),
                             ("http://test.local/XWtN2S889/my20071_/mdavqma1I6M/o9cDWuLgqG_6A/index.html",
                              "//test.local/pogoda/index.html", "/", True),
                             ("//test.local/XWtN2S889/my20071_/mdavqma1I6M/o9cDWuLgqG_6A/index.html",
                              "//test.local/pogoda/index.html", "/", True),
                             ("http://test.local/B6oUGG555/abeed8KMKym/BYlWx8I/aoTf4KVm/KEdp3/hJJD9/L4NQOobaIC-/wLcS2NO/xHCN1/pOuWbejXV3kMpY//querypar?querypar=yes&&json=1",
                              "https://awaps.yandex.net/YkZbKgdWgD.png/querypar?querypar=yes&&json=1", "/", True),
                             ('http://test.local/M1oaV9502/ae186f6srSO/0bBIU/vrFBfkcN/AxEQA-eiV/bl0/1ykVrQB/Y1-KdySI56/Y2tQmG/_GQK2/dH4owWxpHp5/6aFpbJ/dCrX/yqg1mHcns0/ydWtmIh/' +
                              '-SwG2F7/3agI/VP1kWZd/pxT_8hw/g6yRw31r/WiDrc3t/KBbrvC/1WQPCfMOW/S0cWLh7/aveQa/p87A5RSb/Uz-giR/plNKdUWKe5B/_lEOMh5_IVbb/',
                              'https://direct.yandex.ru/?partner', '/', False),
                             ('https://test.local/M1oaV9502/ae186f6srSO/0bBIU/vrFBfkcN/AxEQA-eiV/bl0/1ykVrQB/Y1-KdySI56/Y2tQmG/_GQK2/dH4owWxpHp5/6aFpbJ/dCrX/yqg1mHcns0/ydWtmIh/' +
                              '-SwG2F7/3agI/VP1kWZd/pxT_8hw/g6yRw31r/WiDrc3t/KBbrvC/1WQPCfMOW/S0cWLh7/aveQa/p87A5RSb/Uz-giR/plNKdUWKe5B/_lEOMh5_IVbb/',
                              'https://direct.yandex.ru/?partner', '/', False)
                         ])
def test_decrypt_url(crypted, expected, prefix, is_trailing_slash_enabled):
    decrypted, _, origin = decrypt_url(crypted, TEST_SECRET_KEY, prefix, is_trailing_slash_enabled)
    assert origin is None
    assert expected == decrypted


@pytest.mark.parametrize('crypted, secret_key, prefix, is_trailing_slash_enabled, raised_error',
                         [
                             # пустой урл
                             ("", TEST_SECRET_KEY, "/", False, False),
                             # битый сид
                             ("/XWtN9S102/my200_/mQbvqmJEo8M/sBeR2_L/la-p_d/UVGqH/s381lrh/Ck9Lh5GGE_/KXLzr/N3DvlbQb/hLESeQU/febTuIvVIYZvTSQ/",
                              TEST_SECRET_KEY, "/", True, False),
                             # пустой ключ шифрования
                             ("/XWtN9S102/my20071_/mQbvqmJEo8M/sBeR2_L/la-p_d/UVGqH/s381lrh/Ck9Lh5GGE_/KXLzr/N3DvlbQb/hLESeQU/febTuIvVIYZvTSQ/",
                              "", "/", True, False),
                             # неправильный ключ шифрования
                             ("/XWtN9S102/my20071_/mQbvqmJEo8M/sBeR2_L/la-p_d/UVGqH/s381lrh/Ck9Lh5GGE_/KXLzr/N3DvlbQb/hLESeQU/febTuIvVIYZvTSQ/",
                              "invalid_partner_secret_key", "/", True, False),
                             # часть урла потеряна
                             ("http://test.local/test/a2ZMj3K56/1e8a6f?param1=value1",
                              TEST_SECRET_KEY, "/test/", False, True),
                             ("https://test.local/test/a2ZMj3K56/1e8a6f?param1=value1",
                              TEST_SECRET_KEY, "/test/", False, True),
                             ("test.local/XWtN2S889/my20071_/mdavqma1I6M/o9cDWuLgqG_6A/index.html",
                              TEST_SECRET_KEY, "/", True, False),
                             ("/a9ZMj9K02/1e8a6f2_m_KY/bqDrv-0",
                              TEST_SECRET_KEY, "/", False, True),
                             # зашифрованный урл в квери-параметре не зашифрованного
                             ("//test.local/?url=/a2ZMj3K56/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNdJ0X9DEw", TEST_SECRET_KEY, "/", False, False),

                         ])
def test_decrypt_invalid_url(crypted, secret_key, prefix, is_trailing_slash_enabled, raised_error):
    if raised_error:
        with pytest.raises(DecryptionError):
            decrypt_url(crypted, secret_key, prefix, is_trailing_slash_enabled)
    else:
        assert (None, None, None) == decrypt_url(crypted, secret_key, prefix, is_trailing_slash_enabled)


@pytest.mark.parametrize('args, expected',
                         [
                             ((4, 'ab/c', 'd/e/f'), ('abcd', '/e/f')),
                             ((4, '', 'd/e/fh'), ('defh', '')),
                             ((4, 'a/b//c/', '/d//e/fh'), ('abcd', '//e/fh')),
                             ((16, 'jbssdc/ksjfn?skdj=fs&wf=3&jknd=4'), ('jbssdcksjfnskdjf', 's&wf=3&jknd=4')),
                         ])
def test_resplit_using_length(args, expected):
    assert expected == resplit_using_length(*args)


def test_exception_resplit_using_length():
    with pytest.raises(DecryptionError):
        resplit_using_length(88, 'jbssdc/ksjfn?skdj=fs&wf=3&jknd=4')


@pytest.mark.parametrize('crypted, expected',
                         [
                             ('Ly9hdmF0YXJzLm1kcy55YW5kZXgubmV0L2dldC1hdXRvLzcwNzQ3L2NhdGFsb2cuMjA2ODE5NTkuNzc1MjE0ODYzMjM2Mzg2NDgwL2NhdHRvdWNocmV0',
                              '//avatars.mds.yandex.net/get-auto/70747/catalog.20681959.775214863236386480/cattouchret'),
                             ('Ly95YXN0YXRpYy5uZXQvaWNvbm9zdGFzaXMvXy9VUFh0RnlnQV9IaGpfai13WHJTNlJOTFh5dk0ucG5n',
                              '//yastatic.net/iconostasis/_/UPXtFygA_Hhj_j-wXrS6RNLXyvM.png'),
                             ('aHR0cHM6Ly9hd2Fwcy55YW5kZXgubmV0L1lrWmJLZ2RXZ0QucG5n',
                              'https://awaps.yandex.net/YkZbKgdWgD.png'),
                         ])
def test_decrypt_base64(crypted, expected):
    decrypted = decrypt_base64(crypted)
    assert expected == decrypted


@pytest.mark.parametrize('crypted, expected',
                         [
                             ('fEAMEyIGGAIHZQgdR3ocEhoNCx99AQgRbBUcBFkqEA1be1JDQ11ZSDAOGQQvHR5eRntTQQVtUEpaXllSYV5ZXXVBS0NCeF1PAGxVXBcIGhM8Gg4NMRcN',
                              '//avatars.mds.yandex.net/get-auto/70747/catalog.20681959.775214863236386480/cattouchret'),
                             ('fEAUBDAGGAQdKEsXUSBKGhcGAAggGwwWKgFWL1seNSFAEhwUNTYmDzkwB0g0KgsjQhkrNWwtEz5aGQAA',
                              '//yastatic.net/iconostasis/_/UPXtFygA_Hhj_j-wXrS6RNLXyvM.png'),
                             ('OxsZFTBIVl8VPAQJR3ocEhoNCx99AQgRbCsSKhYAAh1jMyFdBAcJ',
                              'https://awaps.yandex.net/YkZbKgdWgD.png'),
                         ])
def test_decrypt_xor(crypted, expected):
    crypt_key = 'SomeCryptKey4Testing'
    decrypted = decrypt_xor(crypted, crypt_key)
    assert expected == decrypted


def test_get_key():
    key = get_key(TEST_SECRET_KEY, 'my2007')
    assert key.encode('hex') == "f8d6e90f89d2453e5551ee30221be4e5cedb89bb70688c8fb0a9008372d19adc157d12101a4bc59ef2a0d138a40b6ab01697601c92badbd4b445e4412757315f"


@pytest.mark.parametrize('url, prefix, expected',
                         [
                             ("", "/", False),
                             ("yandex.ru/index.html", "/", False),
                             ("yandex.ru/pogoda/index.html", "/pogoda/|/weather/|/", False),
                             ("https://news.yandex.ru/story/V_NATO_prizvali_Rossiyu_pokinut_Krym--d285a316db9e662c0093e20bb871a392?lr=213" +
                              "&lang=ru&stid=qC3ZpiVwt3yQdCKc9uPk&persistent_id=64486452&rubric=index&from=index",
                              "/", False),
                             ("https://auto.ru/moskva/cars/bmw/all/", "/", False),
                             ("https://www.yandex.ru/XWtN9S102/my20071_/mQbvqmJEo8M/sBeR2_L/la-p_d/UVGqH/s381lrh/Ck9Lh5GGE_/KXLzr/N3DvlbQb/hLESeQU/febTuIvVIYZvTSQ/", "/", True),
                             ("https://www.yandex.ru/test/a2ZMj3K56/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNdJ0X9DEw", "/test/|/", True),
                             # зашифрованный урл в квери-параметре не зашифрованного
                             ("//test.local/?url=/a2ZMj3K56/1e8a6fnKKqN5/K86W8G/I61gvLb/BA1AFoo/L-vVHos/BJ_oNdJ0X9DEw", "/", False),
                         ])
def test_is_crypted_url(url, prefix, expected):
    assert expected == is_crypted_url(url, prefix)


def test_decrypt_with_origin():
    _, _, origin = decrypt_url(
        "/SNXt11827/my2007kK/Kdf_roahE0P/8BJQ3WA/gLb1-8/5fHP7/u08Jp7R/X-quwlTSJV/b3692/Z-V4Xqba/gnEf_gO/Mfve5uWF/Grs/AZhVuEKqfrk/bHpiB/NIX-CX0F6iLqR/",
        TEST_SECRET_KEY, "/", False)
    assert origin == "test.local"

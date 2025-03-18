# coding=utf-8
from collections import defaultdict

import pytest
from hamcrest import assert_that, equal_to, has_entries, contains, contains_inanyorder, is_

DEFAULT_URL_SETTINGS = [dict(url="https://yandex.ru/antiadblock", selectors=["some_selector_1", "some_selector_2"]),
                        dict(url="https://yandex.ru/pass_port8080", selectors=["t0l", "Ya"]),
                        dict(url="https://yandex.ru/mlcpp", selectors=["why", "Te", "w0", "lf"]),
                        dict(url=u"https://yandex.ru/video/search?text=мстители", selectors=["a", "b"])]
DEFAULT_GENERAL_SETTINGS = dict(selectors=["maria", "ivan", "sergey", "savelii"])


@pytest.mark.usefixtures("transactional")
def test_argus_get_current_sbs_profile(api, session, service, argus_service_data):
    service_id = service["id"]

    r = session.get(api["service"][service_id]["sbs_check"]["profile"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(data=has_entries(
        url_settings=contains(has_entries(url="https://vk.ru",
                                          selectors=contains("some_select"))))
    ))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize(
    "url_settings, general_settings, status_code", (
        ([{"url": "https://ya.ru/", "selectors": ["some"]}],
         {"selectors": [], "cookies": u"cookie=1"},
         201),
        ([{"url": "https://ya.ru/", "selectors": []}],
         {"cookies": "cookie=1;cookie=2"},
         201),
        ([{"url": "https://ya.ru/", "selectors": ["selectr"]}],
         {"selectors": ["some"]},
         201),
        ([{"url": "https://ya.ru/", "selectors": []}],
         {"cookies": "ola(h)ola"},
         400),
        ([{"url": u"https://yandex.ru/video/search?text=мстители", "selectors": []}],
         {"selectors": [], "cookies": ""},
         201),
        ([{"url": "https://ya.ru/", "selectors": []}],
         {"cookies": "", "selectors": [], "headers": {"X-AAB-EXP-ID": 322, "REQUEST_ID": "some_id"}},
         201),
        ([{"url": "https://ya.ru/", "selectors": []}],
         {"cookies": "", "selectors": [], "headers": []},
         400)
    )
)
def test_params_validation_in_argus(api, session, service, url_settings, general_settings, status_code):
    service_id = service["id"]

    if general_settings:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=url_settings, general_settings=general_settings)))
    else:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=url_settings)))
    assert r.status_code == status_code


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("url_settings, general_settings",
                         (([dict(url="https://ya.ru", selectors=["some1", "some2"]),
                            dict(url="https://yapor.vu")], dict(selectors=["some3"])),
                          ([dict(url="https://vk.ru")], {})))
def test_argus_get_sbs_profile_by_id(api, session, service, url_settings, general_settings):
    service_id = service["id"]

    if general_settings:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=url_settings, general_settings=general_settings)))
    else:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=url_settings)))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["profile"])
    assert r.status_code == 200
    profile_id = r.json()["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS)))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["profile"][profile_id])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(
        id=profile_id,
        service_id=service_id,
        data=has_entries(
            url_settings=equal_to(url_settings),
            general_settings=has_entries(
                **general_settings
            )
        )
    ))


@pytest.mark.usefixtures('transactional')
def test_argus_get_sbs_profile_by_id_errors(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS)))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["profile"])
    assert r.status_code == 200
    profile = r.json()

    r = session.post(api["service"], json=dict(service_id="service.ru",
                                               name="another-service",
                                               domain="service.ru"))
    assert r.status_code == 201
    another_service_id = r.json()["id"]

    r = session.post(api["service"][another_service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    r = session.get(api["service"][another_service_id]["sbs_check"]["profile"])
    assert r.status_code == 200
    another_profile = r.json()

    r = session.get(api["service"][service_id]["sbs_check"]["profile"][another_profile["id"]])
    assert r.status_code == 404
    r = session.get(api["service"][another_service_id]["sbs_check"]["profile"][profile["id"]])
    assert r.status_code == 404

    r = session.get(api["service"][service_id]["sbs_check"]["profile"][100000])
    assert r.status_code == 404


@pytest.mark.usefixtures("transactional")
def test_argus_run_sbs_check(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    # experiment run
    r = session.post(api["service"][service_id]["sbs_check"]["run"],
                     json=dict(exp="123123123"))
    assert r.status_code == 201
    assert r.json()["id"] == 1

    # testing run
    r = session.post(api["service"][service_id]["sbs_check"]["run"],
                     json=dict(testing=True))
    assert r.status_code == 201
    assert r.json()["id"] == 2

    # prod run
    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201
    assert r.json()["id"] == 3

    # also, prod run
    r = session.post(api["service"][service_id]["sbs_check"]["run"],
                     json=dict(testing=False))
    assert r.status_code == 201
    assert r.json()["id"] == 4

    # test run
    r = session.post(api["service"][service_id]["sbs_check"]["run"],
                     json=dict(argus_resource_id=123))
    assert r.status_code == 201
    assert r.json()["id"] == 5

    # invalid params (all params)
    r = session.post(api["service"][service_id]["sbs_check"]["run"],
                     json=dict(exp="123123123", testing=True))
    assert r.status_code == 500


@pytest.mark.usefixtures("transactional")
def test_argus_get_sbs_result_list(api, session, service, tickets):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["profile"])
    assert r.status_code == 200
    profile_id = r.json()["id"]

    r = session.get(api["label"][service_id]["configs"])
    assert r.status_code == 200
    config_id = r.json()["items"][0]["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201
    sandbox_id = r.json()["id"]

    # test run that shouldn't be displayed
    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict(argus_resource_id=123))
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["results"], params=dict(only_my_runs=True))
    assert r.status_code == 200

    data = r.json()["data"]
    assert data['total'] == 1
    assert_that(data['items'][0], has_entries(
        owner="anatoliy",
        config_id=config_id,
        profile_id=profile_id,
        sandbox_id=sandbox_id,
        status="in_progress",
        new_cases=0,
        ok_cases=0,
        problem_cases=0,
        unknown_cases=0,
        obsolete_cases=0,
        no_reference_cases=0
    ))

    push_cases = [
        {'has_problem': 'new'},
        {'has_problem': 'ok'},
        {'has_problem': 'problem'},
        {'has_problem': 'unknown'},
        {'has_problem': 'obsolete'},
        {'has_problem': 'no_reference_case'}
    ]

    r = session.post(api["sbs_check"]["results"],
                     json=dict(status="success",
                               start_time="2019-08-08T10:10:10",
                               end_time="2019-08-08T11:11:11",
                               sandbox_id=sandbox_id,
                               cases=push_cases,
                               filters_lists=['filter_lol', 'filter_kek']),
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["results"])
    assert r.status_code == 200
    data = r.json()['data']
    assert data['total'] == 1

    assert_that(data['items'][0], has_entries(
        owner="anatoliy",
        config_id=config_id,
        profile_id=profile_id,
        sandbox_id=sandbox_id,
        status="success",
        new_cases=1,
        ok_cases=1,
        problem_cases=1,
        unknown_cases=1,
        obsolete_cases=1,
        no_reference_cases=1
    ))


@pytest.mark.usefixtures("transactional")
def test_save_sbs_result(api, session, service, tickets):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS)))
    assert r.status_code == 201

    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201
    sandbox_id = r.json()["id"]

    r = session.post(api["sbs_check"]["results"],
                     json=dict(status="success",
                               start_time="2019-08-08T10:10:10",
                               end_time="2019-08-08T11:11:11",
                               sandbox_id=sandbox_id,
                               cases=['lol', 'tralala'],
                               filters_lists=['filter_lol', 'filter_kek']),
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["results"])
    assert r.status_code == 200
    data = r.json()["data"]
    result_id = data['items'][0]['id']

    r = session.get(api["service"][service_id]["sbs_check"]['results'][result_id])
    assert r.status_code == 200
    assert_that(r.json(), has_entries(
        id=result_id,
        status="success",
        owner="anatoliy",
        sandbox_id=sandbox_id,
        cases=['lol', 'tralala'],
        filters_lists=['filter_lol', 'filter_kek'],
    ))


@pytest.mark.usefixtures("transactional")
def test_get_argus_active_runs(api, session, service, tickets):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201
    sandbox_id = r.json()["id"]

    r = session.get(api["sbs_check"]["runs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["total"] == 1

    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201

    r = session.get(api["sbs_check"]["runs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["total"] == 2

    r = session.post(api["sbs_check"]["results"],
                     json=dict(status="success",
                               start_time="2019-08-08T10:10:10",
                               end_time="2019-08-08T11:11:11",
                               sandbox_id=sandbox_id,
                               cases=['lol', 'tralala'],
                               filters_lists=['filter_lol', 'filter_kek']),
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201

    r = session.get(api["sbs_check"]["runs"], headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert r.json()["total"] == 1


CASES = [
    {
        'headers': {
            'x-aab-requestid': 'request228',
            'X-AAB-EXPID': 322,
        },
        "logs": {
            "elastic-count": {
                "status": "new",
                "url": "",
            },
            "elastic-auction": {
                "status": "new",
                "url": "",
            },
        },
        'adb_bits': 'adb_123',
        'start': '2121-05-11T16:14:28',
        'has_problem': 'new',
        'reference_case_id': 'request1488'
    },
    {
        'headers': {
            'x-aab-requestid': 'request1488',
            'X-AAB-EXPID': 8,
        },
        'logs': {
            "elastic-count": {
                "status": "new",
            },
            "elastic-auction": {
                "status": "new",
            },
        },
        'start': '2121-05-11T16:03:59',
        'adb_bits': 'adb_456',
    },
    {
        'headers': {
            'x-aab-requestid': 'request1941',
            'X-AAB-EXPID': 1945,
        },
        'adb_bits': 'adb_789',
    },
]


def start_and_save_argus_run(api, session, service_id, tickets):
    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201
    sandbox_id = r.json()["id"]

    r = session.post(api["sbs_check"]["results"],
                     json=dict(status="success",
                               start_time="2121-08-08T10:10:10",
                               end_time="2121-08-08T11:11:11",
                               sandbox_id=sandbox_id,
                               cases=CASES,
                               filters_lists=['filter_lol', 'filter_kek']),
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_get_log_status(api, session, service, tickets):
    service_id = service["id"]
    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    start_and_save_argus_run(api, session, service_id, tickets)

    r = session.get(api["sbs_check"]["results"]["logs"],
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), contains_inanyorder(
        has_entries(id=is_(int), adb_bits='adb_456', request_id='request1488'),
        has_entries(id=is_(int), adb_bits='adb_123', request_id='request228'),
    ))


def post_logs_to_success(api, session, tickets, request_ids):
    r = session.get(api["sbs_check"]["results"]["logs"],
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    cases = r.json()
    assert len(cases) == 2
    post_logs = defaultdict(list)
    for case in cases:
        if case["request_id"] in request_ids:
            post_logs[case["id"]].append({
                "request_id": case["request_id"],
                "logs": {
                    "elastic-count": {
                        "status": "success",
                        "url": "fourth_url",
                    },
                    "elastic-auction": {
                        "status": "success",
                        "url": "fifth",
                    }
                }
            })
    r = session.post(api["sbs_check"]["results"]["logs"], json=post_logs,
                     headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 201


@pytest.mark.usefixtures("transactional")
def test_get_adblock_cases_for_verdict(api, session, service, tickets):
    service_id = service["id"]
    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    start_and_save_argus_run(api, session, service_id, tickets)
    post_logs_to_success(api, session, tickets, ["request228", "request1488"])

    r = session.get(api['sbs_check']['results']['verdict_cases'],
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert_that(r.json(), has_entries(
        items=1, data=contains(
            has_entries(request_id="request228", reference_request_id="request1488", id=is_(int))
        )
    ))


@pytest.mark.usefixtures("transactional")
def test_change_argus_log_status(api, session, service, tickets):
    service_id = service["id"]
    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    start_and_save_argus_run(api, session, service_id, tickets)

    post_logs_to_success(api, session, tickets, ["request1488"])
    r = session.get(api["sbs_check"]["results"]["logs"],
                    headers={'X-Ya-Service-Ticket': tickets.CRYPROX_SERVICE_TICKET})
    assert r.status_code == 200
    assert len(r.json()) == 1
    assert_that(r.json(), contains_inanyorder(
        has_entries(id=is_(int), adb_bits='adb_123', request_id='request228')
    ))


@pytest.mark.usefixtures("transactional")
def test_get_logs_from_s3(api, session, service):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=DEFAULT_URL_SETTINGS, general_settings=DEFAULT_GENERAL_SETTINGS)))
    assert r.status_code == 201

    r = session.post(api["service"][service_id]["sbs_check"]["run"], json=dict())
    assert r.status_code == 201
    result_id = r.json()["run_id"]

    logs_type = "bs-dsp-log"
    request_id = "1525eff75d988ef-{cnt}"
    r = session.get(api["sbs_check"]["results"][result_id]["logs"][logs_type]["id"][request_id])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        data=has_entries(
            items=is_(list),
            total=4
        ),
        schema=has_entries(
            dspid="dspid", eventid="eventid", iso_eventtime="iso-eventtime", log_id="log-id", pageid="pageid",
            producttype="producttype", timestamp="timestamp"
        )
    ))


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("url_settings, general_settings, status_code",
                         (([], {}, 400),
                          ([dict(url="vk.com")], {}, 400),
                          ([dict(url="https://vk.ru")], {}, 201),
                          ([dict(url="https://google.com", selectors=['selector']),
                            dict(url="https://hygoogle.ru")], dict(selectors=['selector2']), 201),
                          ('vk.com', 'general', 400)))
def test_argus_create_sbs_profile(api, session, service, url_settings, general_settings, status_code):
    service_id = service["id"]

    r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                     json=dict(data=dict(url_settings=url_settings, general_settings=general_settings),
                               tag="default"))
    assert r.status_code == status_code

    if status_code == 201:
        r = session.get(api["service"][service_id]["sbs_check"]["tag"]["default"])
        assert r.status_code == 200
        profile_ids = r.json()["data"]

        r = session.get(api["service"][service_id]["sbs_check"]["profile"][profile_ids[-1]])
        assert r.status_code == 200
        assert_that(r.json(), has_entries(
            data=has_entries(
                url_settings=equal_to(url_settings),
                general_settings=has_entries(
                    **general_settings
                )
            )
        ))


@pytest.mark.usefixtures("transactional")
def test_get_tags(api, session, service):
    service_id = service["id"]

    default_tag = [
        ([dict(url="https://vk.ru")], {}),
        ([dict(url="https://vk.ru/durov")], {}),
    ]

    argus_test_tag = [
        ([dict(url="https://ya.ru")], {}),
        ([dict(url="https://ya.ru/yaru")], {})
    ]

    for item in default_tag:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=item[0], general_settings=item[1]),
                                   tag="default"))
        assert r.status_code == 201

    for item in argus_test_tag:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=item[0], general_settings=item[1]),
                                   tag="argus.test"))
        assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["tags"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        data=contains_inanyorder('default', 'argus.test'),
        items=2,
    ))


@pytest.mark.usefixtures("transactional")
def test_get_profile_ids_by_tag(api, session, service):
    service_id = service["id"]

    default_tag = [
        ([dict(url="https://vk.ru")], {}),
    ]

    argus_test_tag = [
        ([dict(url="https://ya.ru")], {}),
        ([dict(url="https://ya.ru/yaru")], {}),
        ([dict(url="https://ya.ru/2")], {}),
    ]

    for item in default_tag:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=item[0], general_settings=item[1]),
                                   tag="default"))
        assert r.status_code == 201

    for item in argus_test_tag:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=item[0], general_settings=item[1]),
                                   tag="argus.test"))
        assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["tag"]["default"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        items=1
    ))

    r = session.get(api["service"][service_id]["sbs_check"]["tag"]["argus.test"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        items=1
    ))

    r = session.get(api["service"][service_id]["sbs_check"]["tag"]["argus.test"],
                    params=dict(show_archived=True))
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        items=3
    ))


@pytest.mark.usefixtures("transactional")
def test_archive_tag(api, session, service):
    service_id = service["id"]
    for tag, item in [("default", ([dict(url="https://vk.ru")], {})),
                      ("argus.test", ([dict(url="https://ya.ru")], {}))]:
        r = session.post(api["service"][service_id]["sbs_check"]["profile"],
                         json=dict(data=dict(url_settings=item[0], general_settings=item[1]),
                                   tag=tag))
        assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["tags"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        data=contains_inanyorder('default', 'argus.test'),
        items=2,
    ))

    r = session.patch(api["service"][service_id]["sbs_check"]["tag"]["default"])
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["tags"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        data=contains_inanyorder('argus.test'),
        items=1,
    ))

    r = session.patch(api["service"][service_id]["sbs_check"]["tag"]["argus.test"])
    assert r.status_code == 201

    r = session.get(api["service"][service_id]["sbs_check"]["tags"])
    assert r.status_code == 200

    assert_that(r.json(), has_entries(
        data=equal_to(list()),
        items=0,
    ))

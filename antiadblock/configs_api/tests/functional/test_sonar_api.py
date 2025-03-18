import pytest
from hamcrest import has_entries, is_, assert_that, contains


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize("query_args", ["", "?tags=Opera", "?tags=Opera,desktop"])
def test_sonar_get_rules(api, session, service, query_args):
    service_id = service["id"]
    r = session.get(str(api["service"][service_id]["sonar"]["rules"]) + query_args)
    assert r.status_code == 200

    expected_entries = [
        has_entries(
            added='2019-08-28T14:16:26.672303',
            list_url=['https://dl.opera.com/download/get/?adblocker=adlist&country=ru'],
            raw_rule='@@||ads.adfox.ru/*/getBulk$xmlhttprequest,domain=widgets.kinopoisk.ru|www.kinopoisk.ru',
            is_partner_rule=True,
        )
    ]
    if 'Opera' not in query_args:
        expected_entries += [
            has_entries(
                added='2019-08-07T22:09:37.978809',
                list_url=['https://easylist-downloads.adblockplus.org/advblock+cssfixes.txt'],
                raw_rule='@@||ads.adfox.ru/*/getBulk$xmlhttprequest,domain=www.kinopoisk.ru',
                is_partner_rule=False,
            )
        ]

    assert_that(r.json(), has_entries(
        data=has_entries(
            total=is_(int),
            items=contains(*expected_entries)
        ),
        schema=has_entries(
            added="added",
            list_url="list-url",
            raw_rule="raw-rule",
            is_partner_rule="is-partner-rule"
        )
    ))

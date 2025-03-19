from templaters.tests.utils import FakeUser
from templaters.yandex import YandexMessengerTemplater
ya_msgr_templater = YandexMessengerTemplater()


def test_incident_report_with_missing_assignee():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Хьюстон, у нас проблема",
        "<b>Исполнитель:</b> неизвестен",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/ABC-123'>ABC-123</a>",
        f"<b>Приоритет:</b> высокий",
        f"<b>Компоненты:</b> CMP-1",
    ]

    actual_output_chunks = ya_msgr_templater.generate_incident_output(
        issue_key='ABC-123', summary='Хьюстон, у нас проблема',
        assignee=None, priority='высокий',
        components=['CMP-1']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_empty_fields():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Кажется, что-то пошло не так",
        "<b>Исполнитель:</b> неизвестен",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/XYZ-666'>XYZ-666</a>",
        f"<b>Приоритет:</b> средний",
        f"<b>Компоненты:</b> CMP-1",
    ]
    fake_user = FakeUser(login_tg='', login_staff='')

    actual_output_chunks = ya_msgr_templater.generate_incident_output(
        issue_key='XYZ-666', summary='Кажется, что-то пошло не так',
        assignee=fake_user, priority='средний',
        components=['CMP-1']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_no_staff_login():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "У нас тут юзер без логина на staff",
        "<b>Исполнитель:</b> неизвестен",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/XYZ-666'>XYZ-666</a>",
        f"<b>Приоритет:</b> высокий",
        f"<b>Компоненты:</b> CMP-1",
        '<b>Контакты исполнителя:</b>',
        'https://t.me/shadowBroker',
    ]
    fake_user = FakeUser(login_tg='shadowBroker')

    actual_output_chunks = ya_msgr_templater.generate_incident_output(
        issue_key='XYZ-666', summary='У нас тут юзер без логина на staff',
        assignee=fake_user, priority='высокий',
        components=['CMP-1']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_no_telegram_login():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "У нас тут юзер без логина в telegram",
        "<b>Исполнитель:</b> @HoneyBadger",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/TE-00'>TE-00</a>",
        f"<b>Приоритет:</b> низкий",
        f"<b>Компоненты:</b> BORG-00",
        '<b>Контакты исполнителя:</b>',
        '@HoneyBadger',
        'https://staff.yandex-team.ru/HoneyBadger',
    ]
    fake_user = FakeUser(login_staff='HoneyBadger')

    actual_output_chunks = ya_msgr_templater.generate_incident_output(
        issue_key='TE-00', summary='У нас тут юзер без логина в telegram',
        assignee=fake_user, priority='низкий',
        components=['BORG-00']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_all_fields_filled():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Пожар в датацентре K-128",
        "<b>Исполнитель:</b> @SpiderMan",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/DC-12345'>DC-12345</a>",
        f"<b>Приоритет:</b> высокий",
        f"<b>Компоненты:</b> BORG-00 DC-11",
        '<b>Контакты исполнителя:</b>',
        '@SpiderMan',
        'https://staff.yandex-team.ru/SpiderMan',
        'https://t.me/SuperSpidy90',
    ]
    fake_user = FakeUser(login_staff='SpiderMan', login_tg='SuperSpidy90')

    actual_output_chunks = ya_msgr_templater.generate_incident_output(
        issue_key='DC-12345', summary='Пожар в датацентре K-128',
        assignee=fake_user, priority='высокий',
        components=['BORG-00', 'DC-11']
    )

    assert actual_output_chunks == expected_output_chunks

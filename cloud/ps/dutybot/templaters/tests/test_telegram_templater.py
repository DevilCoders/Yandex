from templaters.telegram import TelegramTemplater
from templaters.tests.utils import FakeUser

telegram_templater = TelegramTemplater()


def test_incident_report_with_missing_assignee():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Boom!",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/KTZ-258'>KTZ-258</a>",
        f"<b>Приоритет:</b> низкий",
        f"<b>Компоненты:</b> TCD-42",
    ]

    actual_output_chunks = telegram_templater.generate_incident_output(
        issue_key='KTZ-258', summary='Boom!',
        assignee=None, priority='низкий',
        components=['TCD-42']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_empty_fields():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Мам, тут молоко снова убежало!",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/WWW-6781'>WWW-6781</a>",
        f"<b>Приоритет:</b> средний",
        f"<b>Компоненты:</b> UFO-66",
    ]
    fake_user = FakeUser(login_tg='', login_staff='', name_staff='')

    actual_output_chunks = telegram_templater.generate_incident_output(
        issue_key='WWW-6781', summary='Мам, тут молоко снова убежало!',
        assignee=fake_user, priority='средний',
        components=['UFO-66']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_no_staff_login():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Найден пользователь без логина на staff",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/USER-1111'>USER-1111</a>",
        f"<b>Приоритет:</b> средний",
        f"<b>Компоненты:</b> ACL",
        '<b>Контакты исполнителя:</b>',
        'https://t.me/CrimsonPie',
    ]
    fake_user = FakeUser(login_tg='CrimsonPie', name_staff='whatever')

    actual_output_chunks = telegram_templater.generate_incident_output(
        issue_key='USER-1111', summary='Найден пользователь без логина на staff',
        assignee=fake_user, priority='средний',
        components=['ACL']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_no_telegram_login():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "Найден пользователь без логина в telegram",
        "<b>Исполнитель:</b> <a href='https://staff.yandex-team.ru/robot-yc-infra'>Робот Дауни-Младший</a>",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/FFT-8000'>FFT-8000</a>",
        f"<b>Приоритет:</b> низкий",
        f"<b>Компоненты:</b> GPG-15",
        '<b>Контакты исполнителя:</b>',
        'https://staff.yandex-team.ru/robot-yc-infra',
    ]
    fake_user = FakeUser(login_staff='robot-yc-infra', name_staff='Робот Дауни-Младший')

    actual_output_chunks = telegram_templater.generate_incident_output(
        issue_key='FFT-8000', summary='Найден пользователь без логина в telegram',
        assignee=fake_user, priority='низкий',
        components=['GPG-15']
    )

    assert actual_output_chunks == expected_output_chunks


def test_incident_report_with_assignee_with_all_fields_filled():
    expected_output_chunks = [
        "<b>Новый инцидент!</b>",
        "[24.07.2022][Infra] Host fail abc01-s3-99.cloud.yandex.net",
        "<b>Исполнитель:</b> <a href='https://staff.yandex-team.ru/jasonrammoray'>Вячеслав Москаленко</a>",
        f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/CLOUDINC-12345'>CLOUDINC-12345</a>",
        f"<b>Приоритет:</b> низкий",
        f"<b>Компоненты:</b> BORG-11 DC-00",
        '<b>Контакты исполнителя:</b>',
        'https://staff.yandex-team.ru/jasonrammoray',
        'https://t.me/JasonRammoray',
    ]
    fake_user = FakeUser(
        login_staff='jasonrammoray',
        login_tg='JasonRammoray',
        name_staff='Вячеслав Москаленко'
    )

    actual_output_chunks = telegram_templater.generate_incident_output(
        issue_key='CLOUDINC-12345', summary='[24.07.2022][Infra] Host fail abc01-s3-99.cloud.yandex.net',
        assignee=fake_user, priority='низкий',
        components=['BORG-11', 'DC-00']
    )

    assert actual_output_chunks == expected_output_chunks

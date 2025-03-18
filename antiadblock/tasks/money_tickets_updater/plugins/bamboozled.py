# coding=utf-8
from urllib import quote
from datetime import timedelta, datetime


URL_TEMPLATE = "https://solomon.yandex-team.ru/?project=Antiadblock&cluster=cryprox-prod&service=bamboozled&l.host=cluster&l.sensor=bamboozled&l.service_id={service_id}&" \
               "l.device=ALL&{sensor}=ALL&l.action={action}&graph=auto&norm=true&b={b}&e={e}&graphOnly=y"

URL_PCODEVER_TEMPLATE = "https://solomon.yandex-team.ru/?project=Antiadblock&cluster=cryprox-prod&service=bamboozled&l.host=cluster&l.sensor=bamboozled_by_pcode&l.service_id={service_id}&" \
                        "l.action={action}&graph=auto&filter=top&filterBy=sum&filterLimit=10&b={b}&e={e}&graphOnly=y"

IFRAME_TEMPLATE = "iframe height='400' width='100%' src='{}' frameborder='0'"
TITLES = ("Adblock type", "Browser")
BAMBOOZLED_ACTIONS = ("try_to_load", "try_to_render", "confirm_block")


def get_bamboozled_info(service_id, date_start):

    date_min = date_start - timedelta(weeks=2)

    content = ""
    for i, sensor in enumerate(("l.browser", "l.app")):
        for action in BAMBOOZLED_ACTIONS:
            url = URL_TEMPLATE.format(service_id=service_id, sensor=sensor, action=action, b=date_min.strftime("%Y-%m-%dT%H:%M:%SZ"), e=datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ"))
            iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
            content += "**{}: {}**\n{}\n".format(TITLES[i], action, iframe)
    # pcodever
    for action in BAMBOOZLED_ACTIONS:
        url = URL_PCODEVER_TEMPLATE.format(service_id=service_id, action=action, b=date_min.strftime("%Y-%m-%dT%H:%M:%SZ"), e=datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ"))
        iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
        content += "**Версии ПКОДа: {}**\n{}\n".format(action, iframe)

    return content

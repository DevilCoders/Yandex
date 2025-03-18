# coding=utf-8
from datetime import timedelta, datetime
from urllib import quote


URL_TEMPLATE = "https://solomon.yandex-team.ru/?project=Antiadblock&cluster=push&service=chevent_stats&l.service_id={service_id}&l.device=_all&l.producttype=_all&l.scale=hour&" \
               "l.sensor={sensor}&graph=auto&b={b}&e={e}&graphOnly=y"

IFRAME_TEMPLATE = "iframe height='400' width='100%' src='{}' frameborder='0'"

TITLES = ("Fraud events", "Bad events")


def get_fraud_info(service_id, date_start):

    date_min = date_start - timedelta(weeks=2)

    content = ""
    for i, sensor in enumerate(("fraud_*", "bad_*")):
        url = URL_TEMPLATE.format(service_id=service_id, sensor=sensor, b=date_min.strftime("%Y-%m-%dT%H:%M:%SZ"), e=datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ"))
        iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
        content += "**{}**\n{}\n".format(TITLES[i], iframe)

    return content

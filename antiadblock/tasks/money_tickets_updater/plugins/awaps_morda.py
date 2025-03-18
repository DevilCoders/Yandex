# coding=utf-8
from datetime import timedelta, datetime
from urllib import quote

URL_SHOWS_TEMPLATE = 'https://charts.yandex-team.ru/preview/AntiAdblock/morda_by_templates?_fg2=1&' \
                     '_graph_json_config={{"graphs":[{{"fields":["{sensor}"],"scale":"h","type":"line","x_field":"fielddate"}}]}}' \
                     '&date_max={date_max}&date_min={date_min}&scale=d_by_h_sum&template_name=_in_table_&visualiser=HighCharts&_embedded=1'

URL_ACTIONS_TEMPLATE = 'https://solomon.yandex-team.ru/?project=Antiadblock&cluster=push&service=morda_stats&' \
                       'service_id=yandex_morda&host=cluster&graph=morda_relative_actions&b={b}&e={e}&graphOnly=y'

IFRAME_TEMPLATE = 'iframe frameborder="0" width="100%" height="400px" src="{}"'

TITLES = ("Показы ААБ", "Показы")


def get_awaps_info(service_id, date_start):
    if service_id != "yandex_morda":
        return

    date_min = date_start - timedelta(weeks=2)

    content = ""

    for i, sensor in enumerate(("aab_shows", "total_shows")):
        url = URL_SHOWS_TEMPLATE.format(sensor=sensor, date_min=date_min.strftime("%Y-%m-%d 00:00:00"), date_max=datetime.now().strftime("%Y-%m-%d 23:59:59"))
        iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
        content += "**{}**\n{}\n".format(TITLES[i], iframe)

    url = URL_ACTIONS_TEMPLATE.format(b=date_min.strftime("%Y-%m-%dT%H:%M:%SZ"), e=datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ"))
    iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
    content += "**Отношение показы/розыгрыши в Авапсе**\n{}\n".format(iframe)

    return content

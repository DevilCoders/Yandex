# coding=utf-8
from datetime import timedelta, datetime
from urllib import quote

URL_TEMPLATE = 'https://charts.yandex-team.ru/preview/AntiAdblock/experiment_bypass_agg?_fg2=1&_graph_json_config={{"graphs":[{{"fields":["{sensor}"],"scale":"i","x_field":"fielddate"}}]}}&' \
               'date_max={date_max}&date_min={date_min}&scale=i&service_id={service_id}&visualiser=HighCharts&_embedded=1'


IFRAME_TEMPLATE = 'iframe frameborder="0" width="100%" height="400px" src="{}"'

TITLES = ("Выигрыши-diff", "Показы-diff", "Деньги-diff")


def get_bypass_experiment_info(service_id, date_start):
    date_min = date_start - timedelta(weeks=2)

    content = ""
    for device in ("DESKTOP", "MOBILE"):
        report_service_id = "{}_{}".format(service_id, device)
        for i, sensor in enumerate(("wins_diff", "shows_diff", "money_diff")):
            url = URL_TEMPLATE.format(service_id=report_service_id, sensor=sensor, date_min=date_min.strftime("%Y-%m-%d 00:00:00"), date_max=datetime.now().strftime("%Y-%m-%d 23:59:59"))
            iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
            content += "**{}**\n{}\n".format("{}_{}".format(TITLES[i], device), iframe)

    return content

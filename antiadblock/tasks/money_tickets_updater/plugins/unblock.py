# coding=utf-8
from datetime import timedelta, datetime
from urllib import quote

URL_TEMPLATE = 'https://charts.yandex-team.ru/preview/AntiAdblock/partners_money_united?_fg2=1&_graph_json_config={{"graphs":[{{"fields":["unblock"],"scale":"d",' \
               '"title":"Процент разблокированных денег",' \
               '"type":"area","x_field":"fielddate"}}]}}&date_min={date_min} 00:00:00&date_max={date_max} 00:00:00&scale=d&service_id={service_id}&visualiser=HighCharts&_embedded=1'

IFRAME_TEMPLATE = 'iframe frameborder="0" width="100%" height="400px" src="{}"'

TITLES = ("Общий", "Desktop", "Mobile", "Direct", "Not Direct")


def get_unblock_info(service_id, date_start):
    services = [s.format(service_id) for s in ("\t{}\t", "\t{}\tdevice\tdesktop\t", "\t{}\tdevice\tmobile\t",
                                               "\t{}\tdsp\tdirect\t", "\t{}\tdsp\tnot_direct\t")]

    # у turbo.yandex.ru нет последнего разреза, у yandex_morda вместо not_direct yandex_morda_awaps
    if service_id == "turbo.yandex.ru":
        del services[4]
    elif service_id == "yandex_morda":
        services[4] = "\tyandex_morda\tdsp\tyandex_morda_awaps\t"

    date_min = date_start - timedelta(weeks=2)
    date_max = datetime.now() - timedelta(days=1)

    content = ""

    for i, service in enumerate(services):
        url = URL_TEMPLATE.format(date_min=date_min.strftime("%Y-%m-%d"), date_max=date_max.strftime("%Y-%m-%d"), service_id=service)
        iframe = "{{" + IFRAME_TEMPLATE.format(quote(url, safe='/:?=%@;&+$,')) + "}}"
        content += "**{}**\n{}\n".format(TITLES[i], iframe)

    return content

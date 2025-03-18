# coding: utf-8
# Исходник бинаря для подсчета потерь денег и иллюстрации потерь на дашборде ANTIADBSUP
# (подсчет основывается на тикетах инцидентов)
import os
from collections import defaultdict
from datetime import datetime, timedelta

from startrek_client import Startrek
from statface_client import StatfaceClient, STATFACE_PRODUCTION

import library.python.charts_notes as notes
from antiadblock.tasks.tools.calendar import Calendar
from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.common_configs import STARTREK_DATETIME_FMT, MSK_UTC_OFFSET

logger = create_logger('support_money_loss')

STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')
STATFACE_TOKEN = os.getenv('STAT_TOKEN')
CHARTS_TOKEN = os.getenv("CHARTS_TOKEN")

MONEY_UNITED_REPORT = 'AntiAdblock/partners_money_united'
SUPPORT_MONEY_LOSS_REPORT = 'AntiAdblock/support_money_loss'
PROBLEMS_TV_FEED = 'antiadblock/problems_graph_new'

MONTH = 31
DATE_FORMAT = '%Y-%m-%d'
DATETIME_FORMAT = '%Y-%m-%d %H:%M:%S'
WEEKEND = 67

calendar = Calendar()
STARTREK_QUERY_TEMPLATE = 'Queue: "ANTIADBSUP" Tags: "negative_trend", "money_drop"{}'


def get_weekday_from_dt(dt):
    """
    :param dt: datetime object
    :return: WEEKEND for weekends and holidays. Else dt.weekday().
    """
    assert isinstance(dt, datetime)
    holidays = calendar.get_nonwork_days()
    return WEEKEND if dt.strftime(DATE_FORMAT) in holidays else dt.weekday()


def get_issue_start_time(issue):
    issue_start_time = issue.incidentStart or issue.createdAt
    return datetime.strptime(issue_start_time.split('.')[0], STARTREK_DATETIME_FMT) + MSK_UTC_OFFSET


def get_issue_end_time(issue):
    issue_end_time = getattr(issue, 'incidentEnd', None) or getattr(issue, 'resolvedAt', None)
    if issue_end_time is not None:
        return datetime.strptime(issue_end_time.split('.')[0], STARTREK_DATETIME_FMT) + MSK_UTC_OFFSET
    else:
        return datetime.now()


def get_money_for_service(service_id, devices, date_to, date_from, scale='h'):
    """
        Загружает данные по деньгам сервиса за период [date_from, date_to]
    """
    report = StatfaceClient(host=STATFACE_PRODUCTION, oauth_token=STATFACE_TOKEN).get_report(MONEY_UNITED_REPORT)
    result = {}
    for device in devices:
        result[device] = report.download_data(
            scale=scale,
            date_max=date_to.strftime(DATETIME_FORMAT),
            date_min=date_from.strftime(DATETIME_FORMAT),
            service_id='\t{}\tdevice\t{}'.format(service_id, device),
            service_id__lvl=2
        )
    return result


def get_devices_from_issue(issue):
    """
        Определяет девайс по тэгу тикета, если тэгов нет, считаем, что тикет на все девайсы
    """
    devices = [tag.split(':')[1] for tag in issue.tags if 'device' in tag]
    return devices or ['desktop', 'mobile']


def calculate_money_for_issue(issue, **period):
    date_to = get_issue_start_time(issue)
    date_from = date_to - timedelta(**period)
    devices = get_devices_from_issue(issue)
    return get_money_for_service(issue.components[0].name, devices, date_to, date_from)


def filter_money_data_by_incidents(money_data, **period):

    def in_interval(dt, start, end):
        """
            Проверяет попадает dt в интервал (start, end)
        """
        return start < dt < end

    for issue in money_data.keys():
        # Найдем все тикеты по партнеру за период, в котором были посчитаны "эталонные" деньги
        # Считаем, что не может существовать двух одновременно открытых тикетов negative_trend на одного партнера
        # Поэтому достаточно найти все тикеты, которые были зарезолвлены в этот период со статусом "Решен"
        # Так как другие статусы означают, что проблемы на самом деле не было и их нужно учитывать при расчете baseline
        issue_start_date = get_issue_start_time(issue)
        for device in money_data[issue].keys():
            past_issues = st_client.issues.find(query=STARTREK_QUERY_TEMPLATE.format(
                ' Resolved: >= "{}" AND Components: {} AND Resolution: fixed AND Tags: "device:{}", empty()'.
                    format((issue_start_date - timedelta(**period)).strftime(DATE_FORMAT), issue.components[0].id, device)))
            for past_issue in past_issues:
                # Для каждого тикета получим его дату начала и закрытия в формате datetime
                past_issue_resolve_date = get_issue_end_time(past_issue)
                past_issue_start_date = get_issue_start_time(past_issue)
                # Оставим в данных только те часы, которые не попадают в интервалы других тикетов
                money_data[issue][device] = filter(
                    lambda i: not in_interval(datetime.strptime(i['fielddate'], DATETIME_FORMAT),
                                              past_issue_start_date,
                                              past_issue_resolve_date),
                    money_data[issue][device])

    return money_data


def calculate_base_line(issues_money_data):
    """
    :param issues_money_data: Исторические данные
    :return: Эталонный процент разблока в формате {issue: {(день недели, час): общие деньги/адблочные деньги}}
    """

    def calculate_base_line_from_money_data(data):
        timeframed_base_line = defaultdict(list)
        for entry in data:
            if not entry['money']:
                continue
            entry_dt = datetime.strptime(entry['fielddate'], DATETIME_FORMAT)
            timeframed_base_line[(get_weekday_from_dt(entry_dt), entry_dt.hour)].append(entry['aab_money'] / entry['money'])
        return timeframed_base_line

    base_line_data = dict()
    for issue in issues_money_data.keys():
        base_line_data[issue] = {}
        for device, data in issues_money_data[issue].items():
            issue_base_line = calculate_base_line_from_money_data(data)
            # Проверим, что данные есть для каждого таймфрейма
            if len(issue_base_line.keys()) != 24 * 6:
                logger.info("Where is no enough money data to calculate baseline for issue {}".format(issue.key))
                # Если данных нет, загрузим ещё 2 месяца и точно так же отфильтруем их по инцидентам
                new_money_data = filter_money_data_by_incidents({issue: calculate_money_for_issue(issue, days=MONTH*4)}, days=MONTH*4)
                # И посчитаем base line еще раз
                issue_base_line = calculate_base_line_from_money_data(new_money_data[issue][device])
            base_line_data[issue][device] = {timeframe: sum(money_data) / len(money_data)
                                             for timeframe, money_data in issue_base_line.items()}
    return base_line_data


def filter_comments_in_charts(date_from, date_to):
    notes_to_delete = notes.fetch(feed=PROBLEMS_TV_FEED, date_from=date_from, date_to=date_to, oauth_token=CHARTS_TOKEN, raw=True)
    for note in notes_to_delete:
        notes.delete(note_id=note["id"], oauth_token=CHARTS_TOKEN)
    logger.info("Filtered comments")


def add_new_comment_by_issue(current_issue):
    date_from = get_issue_start_time(current_issue).strftime(DATE_FORMAT)
    date_to = get_issue_end_time(current_issue).strftime(DATE_FORMAT)
    service_id = current_issue.components[0].name,
    notes.create(
        feed=PROBLEMS_TV_FEED,
        date=date_from,
        date_until=date_to,
        note=notes.Band(text=current_issue.key, color="#ffcc00", visible=True, z_index=0),
        params={"domain": service_id},
        oauth_token=CHARTS_TOKEN
    )


def publish_comments_in_charts(issues):
    date_from = (datetime.now() - timedelta(days=MONTH*6)).strftime(DATE_FORMAT)
    date_to = (datetime.now() + timedelta(days=1)).strftime(DATE_FORMAT)
    filter_comments_in_charts(date_from, date_to)

    for current_issue in issues:
        add_new_comment_by_issue(current_issue)
    logger.info("Comments published")


if __name__ == '__main__':
    st_client = Startrek(useragent='python', token=STARTREK_TOKEN)
    # 1. Загружаем все тикеты с тэгами negative_trend, money_drop за последние полгода
    issues_to_analyze = st_client.issues.find(query=STARTREK_QUERY_TEMPLATE.format(' Created: >= "{}"'.format(
        (datetime.now() - timedelta(days=MONTH*6)).strftime(DATE_FORMAT)
    )))
    logger.info('From {} found {} issues: {}'.format((datetime.now() - timedelta(days=MONTH*6)).strftime(DATE_FORMAT),
                                                     len(issues_to_analyze),
                                                     [i.key for i in issues_to_analyze]))
    # Оставим только открытые тикеты и тикеты с резолюцией решен, отфильтровав "Дубликат" и "Не будет исправлено"
    issues_to_analyze = filter(lambda i: i.resolution is None or i.resolution.key == 'fixed', issues_to_analyze)
    # Добавим комментарии о потерях на графики
    # publish_comments_in_charts(issues_to_analyze)
    # 2. Для каждого тикета загружаем данные по деньгам партнера (поле component) за последние 2 месяца.
    issues_history_money_data = dict()
    logger.info('Calculating history money data')
    for issue in issues_to_analyze:
        issues_history_money_data[issue] = calculate_money_for_issue(issue, days=MONTH*2)
    # 3. Для этих же 2х месяцев найдем все тикеты с тэгами negative_trend, money_drop
    # и выкинем из данных по деньгам периоды, когда они были открыты
    logger.info('Filtering money data by past incidents')
    issues_history_money_data = filter_money_data_by_incidents(issues_history_money_data, days=MONTH*2)
    # 4. Посчитаем эталонный % разблока для каждого таймфрейма (конкретный час конкретного дня недели),
    # как среднее по историческим данным
    #    (Праздники, субботу и воскресенье посчитаем в одном таймфрейме)
    logger.info('Calculating baseline data')
    base_line_money_data = calculate_base_line(issues_history_money_data)
    # 5. Посчитаем разницу между эталонным и текущим процентом разблока для каждого таймфрейма
    #  и применим эту разницу к текущим деньгам партнера, учитывая пропорцию
    # (1%пункт разницы в проценте разблока не означает потерю 1% денег)
    issues_current_money_data = dict()
    # Получим текущие данные по тикетам
    for issue in issues_to_analyze:
        issues_current_money_data[issue] = get_money_for_service(issue.components[0].name, get_devices_from_issue(issue),
                                                                 get_issue_end_time(issue) - timedelta(days=1),
                                                                 get_issue_start_time(issue))

    daily_stat_data = list()
    # В этом словаре будем хранить потери всех сервисов, сгруппированные по датам
    service_all_money_loss = defaultdict(int)
    for issue in issues_current_money_data.keys():
        # В этот словарь запишем потери по дням для конкретного тикета
        loss_data = defaultdict(int)
        for device, data in issues_current_money_data[issue].items():
            # Будем считать потери отдельно по девайсам
            for entry in data:
                timeframe = datetime.strptime(entry['fielddate'], DATETIME_FORMAT)
                base_line_percent = base_line_money_data[issue][device].get((get_weekday_from_dt(timeframe), timeframe.hour), 0)
                base_line_aab_money = (base_line_percent / (1 - base_line_percent)) * (entry['money'] - entry['aab_money'])
                # Если для таймфрейма не посчитался эталонный процент разблока, считаем, что потерь нет
                base_line_aab_money = entry['aab_money'] if base_line_percent == 0 else base_line_aab_money
                # Отрицательные потери считаем равными 0
                money_loss = max(0, (base_line_aab_money - entry['aab_money']))
                # Обрежем время до даты и таким образом просуммируем потери по дням
                loss_data[entry['fielddate'].split()[0]] += money_loss
        for date, loss in loss_data.items():
            # Мы получили словарь вида Дата: Сумма потерь
            # Сформируем из него данные для отправки в стат
            daily_stat_data.append({
                'fielddate': date + ' 00:00:00',
                'loss': loss,
                'service_id': issue.components[0].name,
                'ticket': issue.key
            })
            # Также занесем данные по потерям в специальный словарь для среза ALL
            service_all_money_loss[date + ' 00:00:00'] += loss
    for date, loss in service_all_money_loss.items():
        daily_stat_data.append({
            'fielddate': date,
            'loss': loss,
            'service_id': 'ALL',
            'ticket': 'ALL'
        })
    StatfaceClient(host=STATFACE_PRODUCTION, oauth_token=STATFACE_TOKEN).\
        get_report(SUPPORT_MONEY_LOSS_REPORT).upload_data(scale='d', data=daily_stat_data)

from datetime import datetime
from statistics import mean, median

import arrow
import click
from tqdm import tqdm
import requests

SCHEDULER_LIMIT = 500
TASKS_LIMIT = 500
MIN_TASKS = 3
SANDBOX_API_URL = 'https://sandbox.yandex-team.ru/api/v1.0'
DT_FORMAT = 'Y-m-dTH:M:S'


class DateTime(click.ParamType):
    """The DateTime type converts date strings into `datetime` objects.
    The format strings which are checked are configurable, but default to some
    common (non-timezone aware) ISO 8601 formats.
    When specifying *DateTime* formats, you should only pass a list or a tuple.
    Other iterables, like generators, may lead to surprising results.
    The format strings are processed using ``datetime.strptime``, and this
    consequently defines the format strings which are allowed.
    Parsing is tried using each format, in order, and the first format which
    parses successfully is used.
    :param formats: A list or tuple of date format strings, in the order in
                    which they should be tried. Defaults to
                    ``'%Y-%m-%d'``, ``'%Y-%m-%dT%H:%M:%S'``,
                    ``'%Y-%m-%d %H:%M:%S'``.
    """
    name = 'datetime'

    def __init__(self, formats=None):
        self.formats = formats or [
            '%Y-%m-%d',
            '%Y-%m-%dT%H:%M:%S',
            '%Y-%m-%d %H:%M:%S'
        ]

    def get_metavar(self, param):
        return '[{}]'.format('|'.join(self.formats))

    def _try_to_convert_date(self, value, format):
        try:
            return datetime.strptime(value, format)
        except ValueError:
            return None

    def convert(self, value, param, ctx):
        # Exact match
        for format in self.formats:
            dtime = self._try_to_convert_date(value, format)
            if dtime:
                return dtime

        self.fail(
            'invalid datetime format: {}. (choose from {})'.format(
                value, ', '.join(self.formats)))


def get_schedulers(owner, scheduler_limit=SCHEDULER_LIMIT):
    resp = requests.get(SANDBOX_API_URL + '/scheduler',
                              params={
                                  'owner': owner,
                                  'limit': scheduler_limit})
    resp.raise_for_status()
    return resp.json()['items']


def get_recommendation(scheduler, start, end, tasks_limit=TASKS_LIMIT, min_tasks=MIN_TASKS):
    if scheduler['status'] != 'STOPPED':
        tasks = requests.get(SANDBOX_API_URL + '/task', params={'limit': tasks_limit,
                                                                'created': '..'.join(
                                                                    [start.isoformat(), end.isoformat()]),
                                                                'scheduler': scheduler['id']})
        tasks_items = tasks.json()['items']
        if len(tasks_items) >= min_tasks:
            tasks_times = [t['execution']['current'] for t in tasks_items]
            recommended_timeout = 2 * max(tasks_times)
            scheduler_detailed_response = requests.get(SANDBOX_API_URL + '/scheduler/{}'.format(scheduler['id']))
            if scheduler_detailed_response.status_code == 404:
                return
            scheduler_detailed_response.raise_for_status()
            scheduler_detailed = scheduler_detailed_response.json()
            if 'task' in scheduler_detailed:
                actual_timeout = scheduler_detailed['task']['kill_timeout']
                statistics = min(tasks_times), max(tasks_times), mean(tasks_times), median(tasks_times)
                if actual_timeout > recommended_timeout:
                    recommendations = {
                        'scheduler': scheduler_detailed,
                        'recommended_timeout': int(recommended_timeout),
                        'actual_timeout': actual_timeout,
                        'stats': statistics,
                    }
                    return recommendations


def perform_recommendation(recommendation, sandbox_token):
    headers = {
        'Authorization': 'OAuth '+sandbox_token,
    }
    scheduler = recommendation['scheduler']
    scheduler['task']['kill_timeout'] = recommendation['recommended_timeout']
    resp = requests.put(SANDBOX_API_URL+'/scheduler/{}'.format(scheduler['id']), json=scheduler, headers=headers)
    resp.raise_for_status()


@click.command()
@click.argument('owner')
@click.option('--dry-run', help='Dry run (do not change anything in sandbox)', default=True, type=click.BOOL)
@click.option('--start', help='Start time for analyze', type=DateTime())
@click.option('--end', help='End time for analyze', type=DateTime())
@click.option('--sandbox-token', help='Sandbox token (can also be passed as SANDBOX_OAUTH_TOKEN env var)', envvar="SANDBOX_OAUTH_TOKEN")
def main(owner, dry_run, start, end, sandbox_token):
    if start is None:
        start = arrow.utcnow().shift(months=-1)
    else:
        start = arrow.get(start)

    if end is None:
        end = arrow.utcnow()
    else:
        end = arrow.get(end)

    schedulers = get_schedulers(owner)

    for scheduler in tqdm(schedulers):
        recommendation = get_recommendation(scheduler, start, end)
        if recommendation:
            if dry_run:
                tqdm.write(f'''Recommended kill_timeout for {recommendation["scheduler"]["id"]} is \
{recommendation["recommended_timeout"]}, actual: {recommendation["actual_timeout"]}, stats: {recommendation["stats"]}''')
            else:
                perform_recommendation(recommendation, sandbox_token)

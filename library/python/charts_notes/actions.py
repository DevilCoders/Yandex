from retry.api import retry
import json
import os
import requests

from .comment import Comment
from .utils import remove_nulls, parse_dates, format_utc_time


DEFAULT_HOST = "https://charts.yandex-team.ru/api/v1/comments"
CHARTS_TOKEN_FILE = '~/.charts/token'


def create(
    feed,
    date,
    note,
    date_until=None,
    params=None,
    host=DEFAULT_HOST,
    oauth_token=None,
):
    """
    Create note

    Args:
        * feed - feed name, for example "Users/voidex/offline_eta"
        * date - date to place note on (datetime.date or string)
        * note - note object, see `notes` module
        * date_until - end date, usable only for `Band` note (datetime.date or string)
        * params - additional params
        * host - charts host
        * oauth_token - OAuth token

    Returns id of created note
    """
    date, date_until = parse_dates(date, date_until)
    return create_raw(
        data=prepare_data(feed, date, date_until=date_until, params=params, note=note),
        host=host,
        oauth_token=oauth_token,
    )


@retry(tries=3, delay=1, backoff=3)
def create_raw(data, host=DEFAULT_HOST, oauth_token=None):
    r = requests.post(
        host,
        headers=make_headers(oauth_token),
        data=json.dumps(data),
    )
    r.raise_for_status()
    return r.json()['id']


@retry(tries=3, delay=1, backoff=3)
def modify(
    note_id,
    note,
    feed=None,
    date=None,
    date_until=None,
    params=None,
    host=DEFAULT_HOST,
    oauth_token=None,
):
    """
    Modify note

    Args:
        * note_id - note id to modify
        * feed - feed name, for example "Users/voidex/offline_eta"
        * note - note object, see `notes` module
        * date - date to place note on (datetime.date or string)
        * date_until - end date, usable only for `Band` note (datetime.date or string)
        * params - additional params
        * host - charts host
        * oauth_token - OAuth token

    Returns id of modified note
    """
    date, date_until = parse_dates(date, date_until)
    data = prepare_data(feed, date=date, date_until=date_until, params=params, note=note)
    r = requests.post(
        '{host}/{id}'.format(host=host, id=note_id),
        headers=make_headers(oauth_token),
        data=json.dumps(data),
    )
    r.raise_for_status()
    return r.json()['id']


@retry(tries=3, delay=1, backoff=3)
def delete(
    note_id,
    host=DEFAULT_HOST,
    oauth_token=None,
):
    """
    Delete note

    Args:
        * note_id - note id to delete
        * host - charts host
        * oauth_token - OAuth token
    """
    r = requests.delete(
        '{host}/{id}'.format(host=host, id=note_id),
        headers=make_headers(oauth_token),
    )
    r.raise_for_status()


@retry(tries=3, delay=1, backoff=3)
def fetch(
    feed,
    date_from,
    date_to,
    params=None,
    full_match=True,
    host=DEFAULT_HOST,
    oauth_token=None,
    raw=False,
):
    """
    Fetch comments in date range

    Args:
        * feed - feed name
        * date_from - start date
        * date_to - end date
        * params - params to match
        * full_match - full match params or subset
        * host - charts host
        * oauth_token - OAuth token
        * raw - return raw json result instead of list of `Comment` objects

    Returns: list of `Comment` object or raw json objects (if `raw`)
    """
    date_from, date_to = parse_dates(date_from, date_to)
    query_params = remove_nulls({
        'feed': feed,
        'dateFrom': format_utc_time(date_from),
        'dateTo': format_utc_time(date_to),
        'params': params,
        'matchType': 'full' if full_match else 'contains',
    })
    r = requests.get(
        host,
        headers=make_headers(oauth_token),
        params=query_params,
    )
    return r.json() if raw else [Comment.from_json(j) for j in r.json()]


def prepare_data(feed, date, date_until, params, note):
    data = {
        'feed': feed,
        'date': format_utc_time(date) if date is not None else None,
        'dateUntil': format_utc_time(date_until) if date_until is not None else None,
        'params': params,
    }
    data.update(note.to_json())
    return remove_nulls(data)


def make_headers(oauth_token):
    if oauth_token is None:
        try:
            oauth_token = open(os.path.expanduser(CHARTS_TOKEN_FILE)).read().strip()
        except Exception as e:
            raise RuntimeError('charts token not specified and failed to load from {file} with error: {err}'.format(
                file=CHARTS_TOKEN_FILE,
                err=e,
            ))

    return {
        'Content-Type': 'application/json',
        'Authorization': 'OAuth {}'.format(oauth_token),
    }

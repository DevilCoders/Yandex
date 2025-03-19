import pandas as pd, datetime, ast, re
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from vault_client import instances

def to_int(str_):
    try:
        return int(x)
    except:
        return None

def to_float(str_):
    try:
        return float(x)
    except:
        return None

def apply_types_in_project(schema_):
    apply_types_dict = {}
    for col in schema_:

        if schema_[col] == str:
            apply_types_dict[col] = ne.custom(lambda x: str(x).replace('"', '').replace("'", '').replace('\\','') if x not in ['', None] else '', col)

        elif schema_[col] == int:
            apply_types_dict[col] = ne.custom(lambda x: to_int(x), col)

        elif schema_[col] == float:
            apply_types_dict[col] = ne.custom(lambda x: to_float(x), col)
    return apply_types_dict

def get_yandex_uid(message):
    str_ = message.split(' \"')
    for st in str_:
        if 'yandexuid' in st:
            try:
                return st.split('=')[1].split('\"')[0]
            except:
                return None


def get_host(message):
    str_ = message.split(' \"')
    try:
        return str_[0].split(' ')[5]
    except:
        return None

def get_url(message):
    str_ = message.split(' \"')
    try:
        return str_[1].split(' ')[1]
    except:
        return None

def get_method(message):
    str_ = message.split(' \"')
    try:
        return str_[1].split(' ')[0]
    except:
        return None

def get_referer(message):
    str_ = message.split(' \"')
    try:
        return str_[2].split('\"')[0]
    except:
        return None

def get_login(message):
    str_ = message.split(' \"')
    try:
        if str_[4].split('\"')[0].lower() != '-':
            return str_[4].split('\"')[0].lower()
        else:
            return None
    except:
        return None

def get_useragent(message):
    str_ = message.split(' \"')
    try:
        return str_[3].split('\"')[0]
    except:
        return None

def get_request_id(message):
    str_ = message.split(' \"')
    try:
        return str_[5].split('\"')[1].replace('[', '').replace(']', '')
    except:
        return None

def get_response(message):
    str_ = message.split(' \"')
    try:
        return str_[0].split(' ')[2].replace('[', '').replace(']', '').replace('\n', '')
    except:
        return None

def filter_not_pages(message):
    page = str(get_url(message))
    if '.js' in page:
        return False

    if '.css' in page:
        return False

    if '.png' in page:
        return False

    if '.map' in page:
        return False

    if '.txt' in page:
        return False

    if '.svg' in page:
        return False

    if '.ico' in page:
        return False

    if '.xml' in page:
        return False

    if '.woff' in page:
        return False
    if '.jpg' in page:
        return False

    return True
def get_full_url(url_, host_, method_, referer_):
    if method_ == 'GET':

        if 'api/doc?' in url_:
            url_part = url_.split('=')[1].replace('%2F', '/')
            url_res = 'https://cloud.yandex.ru/docs' + '/' + url_part

            if url_res.endswith('/') or url_res.endswith('?'):
                return url_res[:-1]
            else:
                return url_res

        url_res = 'https://' + host_ + url_

        if url_res.endswith('/') or url_res.endswith('?'):
            return url_res[:-1]
        else:
                return url_res
    else:
        url_res = referer_

        if url_res.endswith('/') or url_res.endswith('?'):
            return url_res[:-1]
        else:
            return url_res

def filter_events(event_, method_):
    if re.match(r'.*/settings.*|.*/get.*|.*/Get.*|.*/list.*|.*/List.*|.*/run.*|.*/simulate.*|.*/check.*|.*/find.*|.*/index.*', event_) and method_ == 'POST':
        return False
    elif method_ == 'GET':
        return False
    else:
        return True


def get_short_url(url_):
    url_ = re.sub('(/[a-z]+[0-9]+[a-z0-9]*|/[0-9]+[a-z]+[a-z0-9]*)', '/id', str(url_))
    url_ = re.sub('(\?invalidResponse=.*|rdisk/.*|&clouds\[\].*|/id:.*|go.php.*|&yclid=.*|\?yclid=.*|undefined\?.*|&keyword=.*|\?keyword=.*|data:.*|&fbclid=.*|\?fbclid=.*|\?mkt_tok=.*|&mkt_tok=.*|\?from=.*|&from=.*|index\..*|&code=.*|\?code=.*|&idkey=.*|\?idkey=.*|&hostName=.*|\?hostName=.*|&ncrnd=.*|\?ncrnd=.*|\?key=.*|&key=.*|\?utm_.*|&utm_.*)', '', str(url_))
    url_ = re.sub('(bucket/[a-z0-9-]+)', 'bucket/', str(url_))
    url_ = re.sub('backup/.*', 'backup/id', str(url_))
    url_ = re.sub('disks/[a-z]+', 'disks/id', str(url_))
    url_ = re.sub('disks/[a-z]+', 'disks/id', str(url_))

    if url_.endswith('/'):
        return url_[:-1]
    else:
        return url_


def get_events(groups):
    for key, records in groups:
        url = ''
        referer = ''
        context_dict = {}
        for rec in records:
            rec_dict = rec.to_dict().copy()

            if rec['method'] == 'GET':
                current_url = get_full_url(rec['url'], rec['host'], rec['method'], rec['referer'])
            else:
                if rec['referer'].endswith('/') or rec['referer'].endswith('?'):
                    current_url = rec['referer'][:-1]
                else:
                    current_url = rec['referer']

            if filter_events(rec['url'], rec['method']):
                rec_dict['event_type'] = 'event'
                rec_dict['event'] = rec['url']
                rec_dict['url'] = current_url
                rec_dict['event_url'] = get_short_url(current_url)
                yield Record(key,**rec_dict)
                continue

            if rec['method'] == 'GET':
                url = current_url
                rec_dict['event_type'] = 'pageview'
                rec_dict['event'] = get_short_url(current_url)
                rec_dict['url'] = current_url
                rec_dict['event_url'] = get_short_url(current_url)
                yield Record(key,**rec_dict)
                continue

            if url != current_url:

                url = current_url
                rec_dict['event_type'] = 'pageview'
                rec_dict['event'] = get_short_url(current_url)
                rec_dict['url'] = current_url
                rec_dict['event_url'] = get_short_url(current_url)

                if current_url not in context_dict:
                    context_dict[current_url] = datetime.datetime.strptime(rec['timestamp'].split('.')[0], '%Y-%m-%dT%H:%M:%S')
                    yield Record(key,**rec_dict)
                else:
                    delta_sec = (datetime.datetime.strptime(rec['timestamp'].split('.')[0], '%Y-%m-%dT%H:%M:%S') - context_dict[current_url]).seconds
                    context_dict[current_url] = datetime.datetime.strptime(rec['timestamp'].split('.')[0], '%Y-%m-%dT%H:%M:%S')
                    if delta_sec <= 58 and delta_sec >= 62:
                        yield Record(key,**rec_dict)
                    else:
                        continue
            else:
                continue

def works_with_logins(mail_):
    if '@' not in str(mail_):
        return str(mail_).lower().replace('.', '-')
    else:
        return str(mail_).lower()

def get_last_not_empty_table(folder_path, job):
    tables_list = sorted([folder_path + '/' + x for x in job.driver.list(folder_path)], reverse=True)
    last_table_rows = 0
    last_table = ''
    for table in tables_list:
        try:
            table_rows = int(job.driver.get_attribute(table, 'chunk_row_count'))
        except:
            continue

        if table_rows > last_table_rows:
            last_table_rows =  table_rows
            last_table = table
    if last_table:
        return last_table
    else:
        return tables_list[0]


def main():
    last_day = str(datetime.date.today()-datetime.timedelta(days = 1))

    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')

    cluster = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool'],

    ).env(

        templates=dict(
            dates='{%s}' % (last_day)
        )
    )

    job = cluster.job()
    clouds = job.table('//home/cloud_analytics/import/console_logs/test') \
        .filter(
            nf.custom(lambda x: str(x) <  last_day, 'timestamp')
        ) \
        .put(
            '//home/cloud_analytics/import/console_logs/test'
        )
    job.run()


    job = cluster.job()
    clouds = job.table('//home/logfeller/logs/qloud-runtime-log/1d/@dates') \
        .filter(
            nf.custom(lambda x: 'cloud' in  str(x), 'qloud_project'),
            nf.custom(lambda x: str(x) in ['production', 'ext-prod'], 'qloud_environment'),
            nf.custom(lambda x: 'python-requests' not in str(x), 'message'),
            nf.custom(lambda x: '/ping' not in str(x), 'message'),
            nf.custom(lambda x: 'GET ' in str(x) or 'POST ' in str(x), 'message'),
            #nf.custom(lambda x: '/api/run ' not in str(x), 'message'),
            nf.custom(lambda x: 'yandexuid=-' not in str(x), 'message'),
            nf.custom(lambda x: 'sandbox.cloud.yandex.net' not in str(x), 'message'),
            #nf.custom(lambda x: 'GET /unistat' not in str(x), 'message'),
            nf.custom(lambda x: 'yandex-team' not in str(x), 'message'),
            nf.custom(lambda x: 'backoffice' not in str(x), 'message'),
            nf.custom(lambda x: 'preprod.cloud' not in str(x), 'message'),
            nf.custom(filter_not_pages, 'message'),
            nf.custom(lambda x: 'supervisord' not in str(x), 'message'),
            #nf.custom(lambda x: 'Request failed' not in str(x), 'message'),
            nf.custom(lambda x: 'yandexuid' in str(x), 'message'),
            #nf.custom(lambda x: '/_/node/getEmptyText' in str(get_url(x)), 'message')
        ) \
        .project(
            'timestamp',
            'message',
            'qloud_environment',
            details = ne.custom(lambda x: str(x).split(' \"'), 'message'),
            yandexuid = ne.custom(get_yandex_uid, 'message'),
            host = ne.custom(get_host, 'message'),
            url = ne.custom(get_url, 'message'),
            method = ne.custom(get_method, 'message'),
            referer = ne.custom(get_referer, 'message'),
            login = ne.custom(get_login, 'message'),
            user_agent = ne.custom(get_useragent, 'message'),
            request_id = ne.custom(get_request_id, 'message'),
            response = ne.custom(get_response, 'message')
        ) \
        .put(
            '//home/cloud_analytics/import/console_logs/test',
            append = True
        )
    job.run()

    schema = {
        "yandexuid": str,
        "timestamp": str,
        "login": str,
        "event": str,
        "event_type": str,
        "event_url": str,
        "host": str,
        "message": str,
        "method": str,
        "qloud_environment": str,
        "referer": str,
        "request_id": str,
        "response": str,
        "url": str,
        "user_agent": str,
        "puid" : str,
        "ts": float
    }

    job = cluster.job()
    cloud_path = get_last_not_empty_table('//home/cloud_analytics/import/iam/cloud_owners/1h', job)


    job = cluster.job()
    clouds = job.table('//home/cloud_analytics/import/console_logs/test') \
    .project(
        ne.all(),
        login = ne.custom(works_with_logins, 'login')
    ) \
    .groupby(
        'yandexuid'
    ) \
    .sort(
        'timestamp'
    ) \
    .reduce(
        get_events
    )
    puid_login = job.table(cloud_path) \
    .filter(
        nf.custom(lambda x: x not in ['', None], 'login')
    ) \
    .unique('login', 'passport_uid') \
    .project(
        login = ne.custom(works_with_logins, 'login'),
        puid = 'passport_uid'
    )

    clouds = clouds \
    .join(
        puid_login,
        by = 'login',
        type = 'left'
    ) \
    .project(
        **apply_types_in_project(schema)
    ) \
    .project(
        ne.all(),
        ts = ne.custom(lambda x: (datetime.datetime.strptime(str(x)[:-1],'%Y-%m-%dT%H:%M:%S.%f') - datetime.datetime(1970,1,1)).total_seconds() , 'timestamp')
    ) \
    .put(
        '//home/cloud_analytics/import/console_logs/events',
        schema = schema
    )
    job.run()

if __name__ == '__main__':
    main()

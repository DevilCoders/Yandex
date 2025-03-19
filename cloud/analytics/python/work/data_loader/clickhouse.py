#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import requests, pandas as pd, re, time

def clickhouse_request(
        host = 'http://mtstat.yandex.ru:8123/',
        user = 'ktereshin',
        password = 'libertin',
        path_to_cert = 'allCAs.pem',
        connection_timeout = 3000,
        query = None
):

    #query_params = '&'.join([key + '=' + params_dict[key] for key in params_dict])
    #host = host + '?' + query_params

    i = 0
    while True:
        if 'https' in host:
            request_obj = requests.post(host, auth=(user, password), params={'query': query}, timeout=connection_timeout, verify=path_to_cert)
        else:
            request_obj = requests.post(host, auth=(user, password), params={'query': query}, timeout=connection_timeout)

        if request_obj.status_code == 200:
            return request_obj
        elif 'DB::Exception' in request_obj.text:
            print('Try %s, Error: \n%s' % (i,request_obj.text))
            break
        else:
            i =+ 1
            print('Try %s, Error: \n%s' % (i,request_obj.text))
            time.sleep(2)
            if i < 10:
                continue
            else:
                break

def get_data_from_ch(request_obj, method = 'full'):
    if method == 'full':
        return request_obj.text
    elif method == 'by_line':
        return request_obj.iter_lines()

def textdata_to_dataframe(_text, query):
    if not 'Code:' in _text:
        if len(_text) > 0:
            columns_text = re.findall(r'select(.*?)from', query.lower().replace('\n', ' '))[0]
            columns = columns_text.strip().split(',')
            if 'tabseparatedwithnames' in query.lower():
                df = pd.DataFrame([row.split('\t') for row in _text.split('\n')[1:]]).iloc[:-1, :]
                df.columns = _text.split('\n')[0].split('\t')
            else:
                df = pd.DataFrame([row.split('\t') for row in _text.split('\n')]).iloc[:-1, :]
                if len(columns) == len(df.columns):
                    df.columns = columns
            return df
        else:
            print('Empty Result')
            return None
    else:
        print('Error')
        print(_text)

def get_clickhouse_data(
        host = 'http://mtstat.yandex.ru:8123/',
        user = 'ktereshin',
        password = 'libertin',
        path_to_cert = 'allCAs.pem',
        connection_timeout = 3000,
        query = None,
        method = 'full'
):
    ch_request = clickhouse_request(
        host = host,
        user = user,
        password = password,
        path_to_cert = path_to_cert,
        connection_timeout = connection_timeout,
        query = query
    )
    text_data = get_data_from_ch(ch_request, method=method)
    if method == 'full':
        real_orders = textdata_to_dataframe(text_data, query)
        return real_orders
    return text_data

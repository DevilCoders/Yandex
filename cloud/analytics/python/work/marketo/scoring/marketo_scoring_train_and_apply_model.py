#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import pandas as pd, numpy as np, catboost, logging, os, sys, requests, datetime
from sklearn.model_selection import train_test_split
from sklearn.utils import shuffle
from sklearn.metrics import confusion_matrix, recall_score, precision_score, roc_auc_score
import scipy.stats as stats
module_path = os.path.abspath(os.path.join('/home/ktereshin/yandex/arcadia/cloud/analytics/python/work'))
if module_path not in sys.path:
    sys.path.append(module_path)

from data_loader import clickhouse
from global_variables import (
    metrika_clickhouse_param_dict,
    cloud_clickhouse_param_dict
)
from nile.api.v1 import (
    clusters,
    aggregators as na,
    extractors as ne,
    filters as nf,
    Record
)
from vault_client import instances

def wait_for_done_running_proccess(os, file_names):
    #lst = lst.split('\n')
    proccess_running = 0
    files_done_dict = {}
    file_names = file_names.split(',')
    while True:
        for file_name in file_names:
            lst = os.popen('ps -ef | grep python').read()
            if file_name in lst:
                print('%s is running' % (file_name))
                time.sleep(30)
            else:
                files_done_dict[file_name] = 1
                proccess_running = 0
                print('%s Done' % (file_name))

        if len(files_done_dict) == len(file_names):
            break

def execute_query(query, cluster, alias, token, timeout=600):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip().split('\n')
    return rows

def chyt_execute_query(query, cluster, alias, token, columns):
    i = 0
    while True:
        try:
            result = execute_query(query=query, cluster=cluster, alias=alias, token=token)
            users = pd.DataFrame([row.split('\t') for row in result], columns = columns)
            return users
        except Exception as err:
            print(err)
            i += 1
            if i > 10:
                print('Break Excecution')
                break

def main():

    files_list = 'marketo_scoring_generate_dataset.py'
    wait_for_done_running_proccess(os, files_list)

    threshold = 0.2

    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool'],

    )

    cluster = 'hahn'
    alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    query = '''
    SELECT
        DISTINCT
        billing_account_id,
        puid
    FROM "//home/cloud_analytics_test/cubes/acquisition_cube/cube"
    WHERE
        event = 'ba_created'
        AND puid != ''
    '''

    columns = ['billing_account_id', 'puid']
    puids = chyt_execute_query(query=query, cluster=cluster, alias=alias, token=token, columns = columns)

    features = cluster_yt.read('//home/cloud_analytics/scoring/learning_dataset').as_dataframe()
    targets = cluster_yt.read('//home/cloud_analytics/scoring/targets').as_dataframe()

    train_data = pd.merge(
        targets[targets['dataset_type'] == 'learning_set'][['puid', 'first_trial_consumption_datetime', 'start_paid_consumption']],
        features,
        on = 'puid',
        how = 'left'
    ).fillna('0')

    to_predict = pd.merge(
        targets[targets['dataset_type'] != 'learning_set'][['puid', 'first_trial_consumption_datetime', 'start_paid_consumption']],
        features,
        on = 'puid',
        how = 'left'
    ).fillna('0')

    train_data = shuffle(train_data).reset_index(drop=True)

    X_train, X_test, y_train, y_test = train_test_split( train_data, train_data['start_paid_consumption'], test_size=0.33)
    X_train, X_eval, y_train, y_eval = train_test_split( X_train, y_train, test_size=0.25)
    positive = X_train[y_train == 1]
    res = pd.DataFrame()
    for i in range(3):
        X_train = pd.concat(
            [
                X_train,
                positive
            ]
        )
        y_train = pd.concat(
            [
                y_train,
                pd.Series([1]*positive.shape[0])
            ]
        )

    cat_col = [
        'ba_payment_type',
        'ba_usage_status',
        'ba_state',
        'device_type',
        'ba_person_type',
        'age',
        'ba_payment_cycle_type',
        'ba_type',
        'channel',
        'country',
        'os',
        'promocode_source',
        'segment',
        'sex',
        'first_trial_consumption_datetime',
        'session_start_time',
        'search_phrase',
        'start_paid_consumption',
        'puid'
    ]
    cat_indexes = []
    for col in cat_col:
        cat_indexes.append(X_train.columns.get_loc(col))

    ignore_col = [
    'first_trial_consumption_datetime','session_start_time','search_phrase','start_paid_consumption', 'puid'
    ]
    ignore_indexes = []
    for col in ignore_col:
        ignore_indexes.append(X_train.columns.get_loc(col))

    site_metrics = []
    other_metrics = []
    for col in X_test:
        if 'count_v' in col or 'tfidf_' in col:
            site_metrics.append(col)
        else:
            other_metrics.append(col)

    features.shape[0]
    site_metric_new = []
    for col in site_metrics:
        temp = features[col].value_counts()[0]
        if features[col].value_counts()[0]/float(features.shape[0]) < 0.95:
            site_metric_new.append(col)

    list_columns = other_metrics + site_metric_new


    X_train['rand'] = np.random.rand(X_train.shape[0])
    X_eval['rand'] = np.random.rand(X_eval.shape[0])
    X_test['rand'] = np.random.rand(X_test.shape[0])
    to_predict['rand'] = np.random.rand(to_predict.shape[0])

    train_pool = catboost.Pool(X_train, y_train, cat_features = cat_indexes)
    eval_pool = catboost.Pool(X_eval, y_eval, cat_features = cat_indexes)
    test_pool = catboost.Pool(X_test, cat_features = cat_indexes)
    predict_pool = catboost.Pool(to_predict, cat_features = cat_indexes)

    learning_rate = 0.5
    subsample = 0.3
    bootstrap_type = 'Bernoulli'
    depth = 1

    model = catboost.CatBoostClassifier(
        iterations=1000,
        depth=depth,
        learning_rate=learning_rate,
        bootstrap_type = bootstrap_type,
        subsample = subsample,
        loss_function='Logloss',
        ignored_features = ignore_indexes,
        verbose=False
    )
    model.fit(train_pool, eval_set = eval_pool, plot = False, early_stopping_rounds = 20,use_best_model = True)

    metris_dict = {
        'confusion_matrix': confusion_matrix(y_test, model.predict(test_pool)),
        'recall': recall_score(y_test, model.predict(test_pool)),
        'precision': precision_score(y_test, model.predict(test_pool)),
        'roc_auc': roc_auc_score(y_test, model.predict(test_pool))
    }
    print('confusion_matrix = \n%s\n' % (metris_dict['confusion_matrix']))
    print('recall = %s\n' % (metris_dict['recall']))
    print('precision = %s' % (metris_dict['precision']))
    print('roc_auc = %s' % (metris_dict['roc_auc']))

    today = str(datetime.date.today()-datetime.timedelta(days = 0))
    metris_dict['date'] = today
    cluster_yt.write('//home/cloud_analytics/scoring/model_quality/metrics', pd.DataFrame([metris_dict]), append=True)

    research = pd.concat(
        [
            X_test.reset_index(drop=True),
            pd.DataFrame(model.predict_proba(test_pool)).rename(columns={0:'not_paid_prob', 1: 'paid_prob'})
        ],
        axis = 1
    )
    research['paid_consumption'] = y_test.reset_index(drop=True)

    research = pd.merge(research,puids,on = 'puid',how = 'left')
    research['prob_cat'] = research['paid_prob'].apply(lambda x: str(int(x*10)*10) + '% - ' + str(int(x*10 + 1)*10) + '%' )

    if len(research.columns[research.isna().any()].tolist()) == 0:
        research['model_date'] = today
        cluster_yt.write('//home/cloud_analytics/scoring/model_quality/test_dataset/%s' % (today), research[['model_date', 'billing_account_id', 'paid_consumption', 'paid_prob', 'prob_cat']+list_columns])


    predict_research = pd.concat(
        [
            to_predict.reset_index(drop=True),
            pd.DataFrame(model.predict_proba(predict_pool)).rename(columns={0:'not_paid_prob', 1: 'paid_prob'})
        ],
        axis = 1
    )
    predict_research = pd.merge(predict_research,puids,on = 'puid',how = 'left')
    predict_research['prob_cat'] = predict_research['paid_prob'].apply(lambda x: str(int(x*10)*10) + '% - ' + str(int(x*10 + 1)*10) + '%' )

    if len(predict_research.columns[predict_research.isna().any()].tolist()) == 0:
        predict_research['model_date'] = today
        cluster_yt.write('//home/cloud_analytics/scoring/model_quality/scored_users/%s' % (today), predict_research[['model_date', 'billing_account_id', 'paid_prob', 'prob_cat']+list_columns])

    predict_research['first_trial_consumption_date'] = predict_research['first_trial_consumption_datetime'].apply(lambda x: str(x).split(' ')[0])

    leads = predict_research[predict_research['first_trial_consumption_date'] == str(datetime.date.today()-datetime.timedelta(days = 8))]
    leads['group'] = leads['first_trial_consumption_date'].apply(lambda x: np.random.choice(['control', 'test']))
    leads_test = leads[leads['group'] == 'test']
    leads_control = leads[leads['group'] == 'control']

    fit_alpha, fit_loc, fit_beta=stats.gamma.fit(leads_test['paid_prob'].values)
    max_ = np.max(leads_test['paid_prob'].values)
    leads_control['score'] = stats.gamma.rvs(fit_alpha, loc=fit_loc, scale=fit_beta, size=leads_control['paid_prob'].shape[0])
    leads_control['score'] = leads_control['score']
    leads_control['score'] = leads_control['score'].apply(lambda x: max_ - np.random.normal(0,0.001, 1)[0] if x > max_ else x )
    leads_test['score'] = leads_test['paid_prob']
    leads_control['score_cat'] = leads_control['score'].apply(lambda x: str(int(x*10)*10) + '% - ' + str(int(x*10 + 1)*10) + '%' )
    leads_test['score_cat'] = leads_test['score'].apply(lambda x: str(int(x*10)*10) + '% - ' + str(int(x*10 + 1)*10) + '%' )
    leads_control['prob_threshold'] =  leads_control['score'].apply(lambda x: 0 if x > threshold else 1)
    leads_test['prob_threshold'] = leads_test['score'].apply(lambda x: 0 if x > threshold else 1)

    leads = pd.concat(
        [
            leads_control,
            leads_test
        ]
    )

    columns = [
        'puid',
        'billing_account_id',
        'first_trial_consumption_date',
        'paid_prob',
        'group',
        'score',
        'score_cat',
        'prob_threshold',
        'prob_cat'
    ]
    leads['model_dict'] = today
    cluster_yt.write('//home/cloud_analytics/scoring/leads/%s' % (today), leads[columns])

if __name__ == '__main__':
    main()

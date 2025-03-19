import pandas as pd, datetime, ast, os,sys, pymysql, logging, requests
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
from sklearn.feature_extraction.text import CountVectorizer, TfidfVectorizer
from gensim.models.doc2vec import Doc2Vec, TaggedDocument

def execute_query(query, cluster, alias, token, timeout=600):
    proxy = "http://{}.yt.yandex.net".format(cluster)
    s = requests.Session()
    url = "{proxy}/query?database={alias}&password={token}".format(proxy=proxy, alias=alias, token=token)
    resp = s.post(url, data=query, timeout=timeout)
    resp.raise_for_status()
    rows = resp.content.strip().split('\n')
    return rows

def get_count_vectors(bag_of_events):
    vectorizer = CountVectorizer(token_pattern=u'[^\t]+')
    vectorizer.fit(list(bag_of_events['event']))

    count_vec_transform = vectorizer.fit_transform(list(bag_of_events['event']))
    return pd.concat(
        [
            bag_of_events[['puid']],
            pd.DataFrame(count_vec_transform.toarray(), columns =vectorizer.get_feature_names())
        ],
        axis = 1
    )

def get_tfidf_vectors(bag_of_events):
    vectorizer = TfidfVectorizer(token_pattern=u'[^\t]+')
    vectorizer.fit(list(bag_of_events['event']))

    count_vec_transform = vectorizer.fit_transform(list(bag_of_events['event']))
    return pd.concat(
        [
            bag_of_events[['puid']],
            pd.DataFrame(count_vec_transform.toarray(), columns =vectorizer.get_feature_names())
        ],
        axis = 1
    )

def get_event2vec(events):
    bag_of_events = events.sort_values(by=['puid', 'timestamp']).groupby(['puid'])['event'].agg(list).reset_index()
    documents = [TaggedDocument(doc, [i]) for i, doc in enumerate(list(bag_of_events['event']))]
    doc2vec_model = Doc2Vec(documents, vector_size=50, window=5, min_count=1, workers=4)
    return doc2vec_model, pd.concat(
        [
            bag_of_events[['puid']],
            pd.DataFrame(list(bag_of_events['event'].apply(lambda x:doc2vec_model.infer_vector(x)))).rename(columns = lambda x: 'event2vec_'+str(x))
        ],
        axis = 1
    )

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


def main():
    files_list = 'marketo_scoring_get_source_data.py'
    wait_for_done_running_proccess(os, files_list)

    client = instances.Production()
    yt_creds = client.get_version('ver-01d33pgv8pzc7t99s3egm24x47')
    cluster_yt = clusters.yt.Hahn(
        token = yt_creds['value']['token'],
        pool = yt_creds['value']['pool'],

    )

    cluster = 'hahn'
    alias = "*ch_public"
    token = '%s' % (yt_creds['value']['token'])

    events = cluster_yt.read('//home/cloud_analytics/scoring/events').as_dataframe()
    events['event'] = events['event'].apply(lambda x: x.lower())
    bag_of_events = events.groupby(['puid'])['event'].agg(lambda x: '\t'.join(list(x))).reset_index()

    count_bag = get_count_vectors(bag_of_events).rename(columns = lambda x: 'count_v_' + str(x) if x != 'puid' else str(x))
    tfidf_bag = get_tfidf_vectors(bag_of_events).rename(columns = lambda x: 'tfidf_' + str(x) if x != 'puid' else str(x))


    doc2vec_model, doc2vec_df = get_event2vec(events)

    result = pd.merge(
        cluster_yt.read('//home/cloud_analytics/scoring/meta_info').as_dataframe(),
        tfidf_bag,
        on = 'puid',
        how = 'left'
    ).fillna(-100)
    result = pd.merge(
        result,
        count_bag,
        on = 'puid',
        how = 'left'
    ).fillna(-100)
    result = pd.merge(
        result,
        doc2vec_df,
        on = 'puid',
        how = 'left'
    ).fillna(-100)


    for col in result.columns:
        if len(col) > 255:
            result.drop(col, axis = 1, inplace = True)


    cluster_yt.write('//home/cloud_analytics/scoring/learning_dataset', result)

if __name__ == '__main__':
    main()

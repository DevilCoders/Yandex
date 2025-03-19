import logging
import pandas as pd
from cloud_antifraud.feature_extraction.names_features import add_names_features
from clan_tools.utils.timing import timing
from clan_tools.utils.cache import cached
import logging
logger = logging.getLogger(__name__)
from sklearn.feature_extraction.text import TfidfVectorizer
import re
import umap

class FeatureExtractor:

    @timing
    #@cached('data/cache/extract_features.pkl')
    def extract_features(self, features:pd.DataFrame):
        
        features.phone = features.phone.astype(int).astype(str)
        features['phone_len'] = features.phone.str.len()
        features['phone_country7'] = (features.phone.str.get(0).astype(int) == 7).astype(int)
        features['phone_oper_code'] = pd.to_numeric(features.phone.str.slice(1, 4), errors='coerce').fillna(0)
        features['phone_oper_code_str'] =  features.phone.str.slice(1, 4)
        features['acc_name_first'] =  features.account_name.str.split().apply(lambda x:x[0])
        features['acc_name_last'] =  features.account_name.str.split().apply(lambda x:x[-1])


        features['ba_time_10min'] = features['ba_time'] // (60*10)
        features['trial_start_10min'] = features['trial_start'] // (60*10)
       
        features = count_cols_freqs(['ba_time_10min', 'trial_start_10min', 'first_name', 'last_name', 'registration_asn', 
                                    'registration_city', 'registration_isp', 'phone_oper_code_str', 'acc_name_first', 'acc_name_last', 
                                    ], features)  
        same_phone_dist_acc = features.groupby(features.phone)['account_name'].nunique()
        same_phone_dist_acc.name = 'same_phone_dist_acc'
        features = features.merge(same_phone_dist_acc, left_on='phone', right_index=True)

        same_acc_name = features.groupby(features.account_name)['billing_account_id'].nunique()
        same_acc_name.name = 'same_acc_names_count'
        features = features.merge(same_acc_name, left_on='account_name', right_index=True)


        features['first_name_in_acc'] = features.apply(lambda x: str(x.first_name) in x.account_name, axis=1)
        features['last_name_in_acc'] = features.apply(lambda x: str(x.last_name) in x.account_name, axis=1)
        features = add_names_features('account_name', features)
        features = add_names_features('ba_name', features)


        ba_count = features.groupby(features.cloud_id)['billing_account_id'].count()
        ba_count.name = 'ba_count'
        features = features.merge(ba_count, left_on='cloud_id', right_index=True)

        features['registration_country'] = (features['registration_country'] == 'Russia').astype(int)
        features['yandex_reg_org'] = (features.registration_org == "yandex llc").astype(int)
        features['match_phone_country'] = features['registration_country'] * features['phone_country7']

        min_created_rules_time = min(features.time)


        rules_cols = [col for col in features.columns.values if ('rule' in col)]
        features.loc[features.trial_start < min_created_rules_time, rules_cols + ['karma', 'is_bad']] = -1


        vectorizer = TfidfVectorizer()
        text_col = features.first_name.astype(str) +  \
                    ' ' + features.last_name.astype(str) +\
                    ' ' + features.account_name.astype(str) +\
                    ' ' + features.ba_name.astype(str).copy() 
        tfidf_word_doc_matrix = vectorizer.fit_transform(text_col.map(lambda x: re.sub(r'\W+', ' ', x)))
        n_comp = 8
        tfidf_embedding = umap.UMAP(metric='hellinger', n_components=n_comp).fit_transform(tfidf_word_doc_matrix)
        for comp in range(n_comp):
            features[f'tfidf{comp}'] = tfidf_embedding[:,comp]

        logger.debug(features.columns.values)
        return features



def count_freqs(col, features):
    features[col+'_freq'] = features.groupby(col)[col].transform('count')#/features.shape[0]
    return features


def count_cols_freqs(cols, features):
    for col in cols:
        features = count_freqs(col, features)
    return features
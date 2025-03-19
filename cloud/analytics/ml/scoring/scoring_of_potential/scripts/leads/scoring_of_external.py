import logging.config
import click

from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter
import os
from category_encoders.target_encoder import TargetEncoder

from clan_tools.data_adapters.YQLAdapter import YQLAdapter
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
import numpy as np
import json

logging.config.dictConfig(default_log_config)

logger = logging.getLogger(__name__)



@click.command()
@click.option('--onboarding_leads')
@click.option('--result_path')
def score_external(onboarding_leads: str, result_path:str):
    data = YQLAdapter().execute_query(f'''--sql
        SELECT
            inn, 
            main_okved2_name, 
            workers_range, 
            company_size_revenue, 
            legal_city, 
            okato_region_name,
            company_size_description,
            egrul_likvidation,
            visitted,
            levels_diff,
            max_level,
            min_level,
            spark_phones_count,
            puids_count,
            match,
            has_domain_in_spark,
            segment_enterprise,
            segment_medium,    
            segment_mass,
            first_okved
        FROM `{onboarding_leads}`;
    ''', to_pandas=True)
        

    X = data[['main_okved2_name', 
            'workers_range', 
            'company_size_revenue', 
            'legal_city', 
            'okato_region_name',
            'company_size_description',
            'egrul_likvidation',
            'visitted',
            'max_level',
            'min_level',
            'spark_phones_count',
            'puids_count',
            'has_domain_in_spark',
            'levels_diff',
            'first_okved'
            ]]




    target_columns = ['segment_enterprise',
                    'segment_medium',    
                    'segment_mass',
                    'match']

    y = data[target_columns].astype(int)

    high_cardinality_cols=['main_okved2_name', 
                        'workers_range', 
                        'legal_city', 
                        'okato_region_name', 
                        'company_size_description',
                        'first_okved',
                        'egrul_likvidation']
    binary = ['visitted', 'has_domain_in_spark']
    numerical = ['company_size_revenue', 
                'max_level',
                'min_level',
                'spark_phones_count',
                'puids_count',
                'levels_diff']




    target_encoder = TargetEncoder()
    X_encoded = X[binary + numerical]




    for target in target_columns:
        target_encoder.fit(X[high_cardinality_cols], y[target])
        X_target = target_encoder.transform(X[high_cardinality_cols])
        X_target.columns += '_' + target
        X_encoded = X_encoded.join(X_target)


    clf = RandomForestClassifier(max_depth=4, min_samples_leaf=5, n_estimators=50, n_jobs=-1)
    X_train, X_test, y_train, y_test = train_test_split(X_encoded, y, test_size=0.33, random_state=42)
    clf.fit(X_train, y_train)
    y_test_pred = clf.predict_proba(X_test)
    y_test_pred=np.array(list(map(lambda x: x[:, 1], y_test_pred))).T
    y_test_pred = (y_test_pred == np.max(y_test_pred, axis=1, keepdims=1)).astype(int)




    # sum(y_test_pred[:,0][y_test.segment_enterprise==0]), (y_test.segment_enterprise==0).sum()
    # sum(y_test_pred[:,0][y_test.segment_enterprise==1]), y_test.segment_enterprise.sum(), sum(y_test_pred[:,0])
    # sum(y_test_pred[:,1][y_test.segment_medium==1]), y_test.segment_medium.sum()
    # sum(y_test_pred[:,2][y_test.segment_mass==1]), y_test.segment_mass.sum()
    # sum(y_test_pred[:,3][y_test.match==1]), y_test.match.sum()

    

    clf.fit(X_encoded, y)
    y_pred = clf.predict_proba(X_encoded)
    preds=np.array(list(map(lambda x: x[:, 1], y_pred))).T



    data[target_columns] = preds

    mass_ltv = 20*7
    medium_ltv = 365
    enterprise_ltv = 365*3

    mass_daily = 8
    medium_daily = 3000
    enterprise_daily = 7000




    score = mass_ltv*mass_daily*data.segment_mass + medium_ltv*medium_daily*data.segment_medium + enterprise_ltv*enterprise_daily*data.segment_enterprise
    data['score'] = score.values

    YTAdapter().save_result(result_path, schema=None, 
                            df=data[['inn', 
                                 'score', 
                                 'segment_mass', 
                                 'segment_medium',
                                 'segment_enterprise'
                            ]], append=False)

    with open('output.json', 'w') as f:
        json.dump({"result_table": result_path}, f)
        

        

if __name__ == '__main__':
    score_external()
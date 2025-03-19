import nirvana.job_context as nv
import tarfile
from datetime import timedelta
import os
from dataforge import base, datasource, client, objects, pricing
import pandas as pd
import logging.config
import markdown2
from dataforge import classification, classification_loop, control
import yaml
import logging.config
import json
import numpy as np
import shutil


job_context = nv.context()
inputs = job_context.get_inputs()
outputs = job_context.get_outputs()
nv_params = job_context.get_parameters()


tar = tarfile.open(inputs.get('instructions'))
tar.extractall()
tar.close()

pd.set_option('display.max_colwidth', 1000)

with open('logging.yaml') as f:
    logging.config.dictConfig(yaml.full_load(f.read()))

lang = nv_params.get('lang')
token = nv_params.get('toloka-token')
toloka_client = client.create_toloka_client(token)

def read_localized(path):
    dir_localized = 'localized-sbs'
    result = {}
    for (dirpath, dirnames, filenames) in os.walk(f'{dir_localized}/'):
        worker_langs = dirnames
        break

    for worker_lang in worker_langs:
        if '.' in worker_lang:
            continue
        with open(f'{dir_localized}/{worker_lang}/{path}.md') as f:
            result[worker_lang] = markdown2.markdown(f.read())

    return base.LocalizedString(result)


function=base.SbSFunction(inputs=(objects.Audio,), hints=(objects.Text,))

instruction = read_localized('instructions')

name, description = read_localized('name'), read_localized('description')
task_spec = client.TaskSpec(
    id=f'[DataForge][{lang}]Side by side',
    function=function,
    name=name,
    description=description,
    instruction=instruction)


task_spec_p = client.PreparedTaskSpec(task_spec, lang)

input_objects = datasource.read_tasks(inputs.get('tasks'), task_spec_p.task_mapping)
control_objects = datasource.read_tasks(inputs.get('control_tasks'), task_spec_p.task_mapping, has_solutions=True)

task_duration_hint = timedelta(seconds=20)

pricing_config, params = client.LoopParams.default_pricing_and_quality_params(
    task_duration_hint, task_spec_p, plot_pricing_options=False)

correct_control_task_ratio_for_acceptance = 0.7

pricing_config_with_properties = pricing.calculate_properties_for_pricing_config(
    config=pricing_config,
    task_duration_hint=task_duration_hint,
    correct_control_task_ratio_for_acceptance=correct_control_task_ratio_for_acceptance,
    task_mapping=task_spec_p.task_mapping)

print(f'your pricing config properties:\n'
      f'\tprice per hour: {pricing_config_with_properties.price_per_hour:.2f}$\n'
      f'\trobustness: {pricing_config_with_properties.robustness:.3f}')

params.overlap = classification_loop.DynamicOverlap(min_overlap=3, max_overlap=5, confidence=.85)

with open('logging.yaml') as f:
    logging.config.dictConfig(yaml.full_load(f.read()))

client.define_task(task_spec_p, toloka_client)

raw_results, worker_weights = client.launch_sbs(
    task_spec_p,
    client.LoopParams(
        pricing_config=pricing_config,
        task_duration_hint=task_duration_hint,
        params=params),
    input_objects,
    control_objects,
    toloka_client,
    interactive=False)

results = client.ClassificationResults(input_objects, raw_results, task_spec_p, worker_weights)

with open(inputs.get('tasks'), "r") as read_file:
    tasks = json.load(read_file)

link_to_speaker = {}

for task in tasks:
    link_to_speaker[task['audio_a']] = task['speaker']

predict = results.predict()
predict_proba = results.predict_proba()
worker_labels = results.worker_labels()

predict['speaker'] = predict['audio_a'].map(link_to_speaker)
predict_proba['speaker'] = predict_proba['audio_a'].map(link_to_speaker)
worker_labels['speaker'] = worker_labels['audio_a'].map(link_to_speaker)

predict.to_csv(outputs.get('predict'))
predict_proba.to_csv(outputs.get('predict_proba'))
worker_labels.to_csv(outputs.get('worker_labels'))


def aggregate(data):
    aggregated = {}
    df = data.reset_index()
    index = 0
    for _, item in df.iterrows():
        sample_id = index
        if sample_id not in aggregated:
            aggregated[sample_id] = [0, 0]
        aggregated[sample_id][0] += int(item["label"] == "a")
        aggregated[sample_id][1] += 1
        index += 1

    return aggregated


def main(in1, in2):
    data = aggregate(in1).values()
    data1 = [sample_id[0] / sample_id[1] for sample_id in data]
    data2 = [int(sample_id[0] / sample_id[1] > 0.5) for sample_id in data]
    data3 = [int(item["result"] == "a") for _, item in in2.iterrows()]

    metric1, metric2, metric3 = np.mean(data1), np.mean(data2), np.mean(data3)
    result1, result2, result3 = [], [], []
    n_resamples = 1000
    for _ in range(n_resamples):
        sample1 = np.random.choice(data1, size=len(data), replace=True).tolist()
        sample2 = np.random.choice(data2, size=len(data), replace=True).tolist()
        sample3 = np.random.choice(data3, size=len(data), replace=True).tolist()
        val1, val2, val3 = np.mean(sample1), np.mean(sample2), np.mean(sample3)
        result1.append(val1)
        result2.append(val2)
        result3.append(val3)
    alpha = 0.025

    return {
               "metric1": {
                   "value": metric1,
                   "CI": [np.quantile(result1, alpha), np.quantile(result1, 1 - alpha)]
               },
               "metric2": {
                   "value": metric2,
                   "CI": [np.quantile(result2, alpha), np.quantile(result2, 1 - alpha)]
               },
               "metric3": {
                   "value": metric3,
                   "CI": [np.quantile(result3, alpha), np.quantile(result3, 1 - alpha)]
               }
           }


unique_speakers = predict['speaker'].unique()

res = {}

for speaker in unique_speakers:
    cur_predict = predict[predict['speaker'] == speaker]
    cur_worker_labels = worker_labels[worker_labels['speaker'] == speaker]

    cur_res = main(cur_worker_labels, cur_predict)
    res[speaker] = cur_res

with open(outputs.get('metrics'), 'w', encoding='utf-8') as f:
    json.dump(res, f, ensure_ascii=False, indent=4)

shutil.copy('./dataforge.log', outputs.get('dataforge.log'))

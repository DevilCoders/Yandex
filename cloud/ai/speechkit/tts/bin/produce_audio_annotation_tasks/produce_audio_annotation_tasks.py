import nirvana.job_context as nv
import yt.wrapper
import json
import random


def toloka_format(tasks, question_config):
    tasks_suits = []
    for sample in tasks:
        inputs = {
            "input_values": {
                "question": question_config["question"],
                "options": question_config["options"]
            }
        }

        inputs["input_values"]["sample_id"] = sample["ID"]
        for field in question_config["fields"]:
            inputs["input_values"][field] = sample[field]

        if "golden" in sample:
            inputs["known_solutions"] = [{
                "correctness_weight": 1,
                "output_values": {
                    "result": sample["golden"]
                }
            }]

        tasks_suits.append(inputs)
    tasks_suits[0]["input_values"]["alert"] = f'Следующий вопрос: {question_config["question"]}'
    return tasks_suits


def collect_to_toloka_format(tasks, honeypots, question_config, question, mixer_config, overlap):

    tasks_suits = []
    for index in range(0, len(tasks), mixer_config["tasks"]):
        tasks_suit = []

        tasks_suit += toloka_format(tasks[index: index + mixer_config["tasks"]], question_config[question])
        tasks_suit += toloka_format(random.sample(honeypots, mixer_config["golden"]), question_config["sbs"])
        tasks_suits.append({
            "tasks": tasks_suit,
            "overlap": overlap
        })

    return tasks_suits


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()
    questions_def_path = inputs.get('questions_def')
    honeypots_path = inputs.get('honeypots')
    mixer_config_path = inputs.get('mixer_config')
    tasks_path = outputs.get('tasks')
    table_path = parameters.get('table')
    bucket_path = parameters.get('bucket-path')
    question = parameters.get('question')
    overlap = parameters.get('overlap')

    categories = parameters.get('categories')
    damage_amounts = parameters.get('damage-amounts')

    # Говорим библиотеке что мы будем работать с кластером hahn.
    yt.wrapper.config.set_proxy("hahn")

    # Читаем информацию о типах ошибок.
    info = {}
    for row in yt.wrapper.read_table(bucket_path):
        info[row["ID"]] = {
            "category": row["category"].split('.')[1],
            "damage_amount": row["damage_amount"].split('.')[1],
        }

    # Читаем основные задания
    tasks = []
    for row in yt.wrapper.read_table(table_path):
        # фильтруем
        if info[row["ID"]]["category"] not in categories\
         or info[row["ID"]]["damage_amount"] not in damage_amounts:
            continue
        tasks.append(row)

    # Читаем контрольные задания
    with open(honeypots_path, 'r') as f:
        honeypots = json.load(f)

    # Читаем метаданные
    with open(questions_def_path, 'r') as f:
        question_config = json.load(f)
    with open(mixer_config_path, 'r') as f:
        mixer_config = json.load(f)
    tasks_suits = collect_to_toloka_format(tasks, honeypots, question_config, question, mixer_config, overlap)

    with open(tasks_path, 'w') as result_file:
        json.dump(tasks_suits, result_file, indent=4, ensure_ascii=False)

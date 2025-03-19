import nirvana.job_context as nv
import yt.wrapper as yt
import json
import uuid

from datetime import datetime
from dataclasses import dataclass

from cloud.ai.lib.python.datasource.yt.model import objects_to_rows, TableMeta, generate_attrs, get_table_name,\
    generate_json_options_for_table
from cloud.ai.lib.python.datetime import now
from cloud.ai.lib.python.serialization import OrderedYsonSerializable


@dataclass
class AudioAnnotation(OrderedYsonSerializable):
    id: str  # UUID
    assignment_id: str
    audio_id: str
    question: str
    answer: bool
    category: str
    damage_amount: str
    received_at: datetime

    @staticmethod
    def primary_key():
        return ['audio_id', 'id']


table_audio_annotations_meta = TableMeta(
    dir_path='//home/mlcloud/speechkit/tts/audio_annotation_experiment/audio_annotations',
    attrs=generate_attrs(AudioAnnotation, required={'received_at'}),
)


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()
    tasks_path = inputs.get('tasks.json')
    question = parameters.get('question')
    audio_info_path = parameters.get('audio-info-path')

    # Говорим библиотеке что мы будем работать с кластером hahn.
    yt.config.set_proxy("hahn")

    # Читаем информацию о типах ошибок.
    audio_id_to_info = {}
    for row in yt.read_table(audio_info_path):
        audio_id_to_info[row["ID"]] = {
            "category": row["category"],
            "damage_amount": row["damage_amount"],
        }

    with open(tasks_path, 'r') as f:
        tasks = json.load(f)

    received_at = now()
    audio_annotations = []
    for task in tasks:
        if task["knownSolutions"]:
            continue

        audio_id = task["inputValues"]["sample_id"]
        audio_annotations.append(
            AudioAnnotation(
                id=str(uuid.uuid4()),
                assignment_id=task["assignmentId"],
                audio_id=audio_id,
                question=question,
                answer=task["outputValues"]["result"],
                received_at=received_at,
                category=audio_id_to_info[audio_id]["category"],
                damage_amount=audio_id_to_info[audio_id]["damage_amount"]
            )
        )

    with open(outputs.get('audio_annotations_rows.json'), 'w') as f:
        json.dump(objects_to_rows(audio_annotations), f, indent=4, ensure_ascii=False)

    with open(outputs.get('audio_annotations_table.json'), 'w') as f:
        json.dump(generate_json_options_for_table(table_audio_annotations_meta, get_table_name(received_at)),
                  f, indent=4, ensure_ascii=False)

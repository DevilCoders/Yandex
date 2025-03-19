from multiprocessing import Process
import math
import typing
import ujson as json

from ..model import TextTransformationStep, EvaluationTarget, MetricsConfig
from .common import TextTransformer
from .lemmatizer import Lemmatizer
from .normalizer import Normalizer
from .punctuation import PunctuationCleaner


def transform_evaluation_targets(
    evaluation_targets_path: str, metrics_config: MetricsConfig, normalizer_data_path: str
) -> typing.Dict[typing.Tuple, str]:
    with open(evaluation_targets_path) as f:
        source_evaluation_targets = [EvaluationTarget.from_yson(r) for r in json.loads(f.read())]

    text_transformers = create_text_transformers(normalizer_data_path)

    text_transformation_cases_set = set([])
    for text_process_cases in metrics_config.metric_name_to_text_transformation_cases.values():
        for text_transformation_case in text_process_cases:
            text_transformation_cases_set.add(tuple(text_transformation_case))

    text_transformation_cases_list = sorted(text_transformation_cases_set, key=lambda case: len(case))

    text_transformation_case_to_evaluation_targets_path = {}
    for text_transformation_case in text_transformation_cases_list:
        skip_steps = 0
        if len(text_transformation_case) == 0:
            text_transformation_case_to_evaluation_targets_path[text_transformation_case] = evaluation_targets_path
            continue
        # TODO: generalize transformed texts reuse
        elif len(text_transformation_case) > 1 and text_transformation_case[0] == TextTransformationStep.Normalize:
            print('reusing evaluation targets of previously transformed "%s"' % TextTransformationStep.Normalize.value)
            skip_steps = 1
            with open(text_transformation_case_to_evaluation_targets_path[(TextTransformationStep.Normalize,)]) as f:
                evaluation_targets = [EvaluationTarget.from_yson(r) for r in json.loads(f.read())]
        else:
            evaluation_targets = [r.clone() for r in source_evaluation_targets]
        text_transformation_case_to_evaluation_targets_path[text_transformation_case] = process_transformation_case(
            evaluation_targets,
            text_transformation_case,
            text_transformers,
            skip_steps,
        )

    return text_transformation_case_to_evaluation_targets_path


def process_transformation_case(
    evaluation_targets: typing.List[EvaluationTarget],
    text_transformation_case: typing.Tuple,
    text_transformers: typing.Dict[TextTransformationStep, TextTransformer],
    skip_steps: int,
) -> str:
    text_transformation_case_str = get_transformation_case_str(list(text_transformation_case))
    print('processing text transformation case "%s"' % text_transformation_case_str)

    def transform(evaluation_targets_part: typing.List[EvaluationTarget], store_path: str):
        for evaluation_target in evaluation_targets_part:
            evaluation_target.hypothesis = text_transformer.transform(evaluation_target.hypothesis)
            evaluation_target.reference = text_transformer.transform(evaluation_target.reference)
        with open(store_path, 'w') as f:
            f.write(json.dumps([r.to_yson() for r in evaluation_targets_part], ensure_ascii=False))

    text_transformation_case = text_transformation_case[skip_steps:]

    for text_transformation_step in text_transformation_case:
        text_transformer = text_transformers[text_transformation_step]
        process_count = 8

        def evaluation_targets_chunks():
            chunk_size = math.ceil(len(evaluation_targets) / process_count)
            for chunk_index in range(0, len(evaluation_targets), chunk_size):
                yield evaluation_targets[chunk_index : chunk_index + chunk_size]

        processes = []
        store_paths = []
        for i, evaluation_targets_chunk in enumerate(evaluation_targets_chunks()):
            store_path = 'tmp_evaluation_targets_chunk_%d.json' % i
            store_paths.append(store_path)
            process = Process(target=transform, args=(evaluation_targets_chunk, store_path))
            process.start()
            processes.append(process)

        for process in processes:
            process.join()

        evaluation_targets = []
        for store_path in store_paths:
            with open(store_path) as f:
                evaluation_targets_chunk = json.loads(f.read())
                for evaluation_target in evaluation_targets_chunk:
                    evaluation_targets.append(EvaluationTarget.from_yson(evaluation_target))

    transformed_evaluation_targets_path = 'evaluation_targets_%s.json' % text_transformation_case_str
    with open(transformed_evaluation_targets_path, 'w') as f:
        f.write(json.dumps([r.to_yson() for r in evaluation_targets], ensure_ascii=False))

    return transformed_evaluation_targets_path


def create_text_transformers(normalizer_data_path: str) -> typing.Dict[TextTransformationStep, TextTransformer]:
    lemmatizer = Lemmatizer()
    normalizer = Normalizer(normalizer_data_path=normalizer_data_path, model='revnorm', language_code='ru-RU')
    punctuation_cleaner = PunctuationCleaner()
    return {text_processor.name(): text_processor for text_processor in [lemmatizer, normalizer, punctuation_cleaner]}


def get_transformation_case_str(text_transformation_case: typing.List[TextTransformationStep]) -> str:
    return '_'.join(c.value for c in text_transformation_case)

import nirvana.job_context as nv
import tarfile
from datetime import timedelta
import toloka.client as toloka
from dataforge import base, datasource, classification, classification_loop, client, control, evaluation, mapping, metrics, objects, pricing, project
import yaml
import logging.config
with open('logging.yaml') as f:
    logging.config.dictConfig(yaml.full_load(f.read()))
import numpy as np
import json
import pandas as pd
from scipy.stats import t
from markdown2 import Markdown as _Markdown
from toloka.client.user_restriction import UserRestriction
from dataforge import utils
from dataforge import pool as pool_config
from dataforge.client import PreparedTaskSpec, LoopParams, check_project, validate_objects_volume, get_pool_price, ask, find_training_requirement
import shutil


job_context = nv.context()
inputs = job_context.get_inputs()
outputs = job_context.get_outputs()
nv_params = job_context.get_parameters()


tar = tarfile.open(inputs.get('instructions'))
tar.extractall()
tar.close()

lang = nv_params.get('lang')
token = nv_params.get('toloka-token')
toloka_client = client.create_toloka_client(token)

with open(inputs.get('tasks'), "r") as read_file:
    tasks = json.load(read_file)

syn_audios = [(objects.Audio(sample['audio']),) for sample in tasks]

logger = logging.getLogger('dataforge')

with open('./localized-mos/description.json', "r") as read_file:
    description = json.load(read_file)


class MOS(base.Evaluation, base.Class):
    BAD = '1'
    POOR = '2'
    FAIR = '3'
    GOOD = '4'
    EXCELLENT = '5'

    @classmethod
    def labels(cls):
        return {
            cls.EXCELLENT: {cur_lang: description['answer_EXCELLENT'][cur_lang] for cur_lang in description['answer_EXCELLENT']},
            cls.GOOD: {cur_lang: description['answer_GOOD'][cur_lang] for cur_lang in description['answer_GOOD']},
            cls.FAIR: {cur_lang: description['answer_FAIR'][cur_lang] for cur_lang in description['answer_FAIR']},
            cls.POOR: {cur_lang: description['answer_POOR'][cur_lang] for cur_lang in description['answer_POOR']},
            cls.BAD: {cur_lang: description['answer_BAD'][cur_lang] for cur_lang in description['answer_BAD']},
        }

function = base.ClassificationFunction(inputs=(objects.Audio,), cls=MOS)

class Markdown(_Markdown):
    def postprocess(self, text: str) -> str:
        for align in ['left', 'right', 'center']:
            text = text.replace(f'"text-align:{align};"', f'"text-align:{align}"')
        return text

instruction = {}
with open(f'./localized-mos/mos-instruction_{lang}.md') as f:
    instruction[lang] = Markdown(extras=["tables"]).convert(f.read())

name = base.LocalizedString({cur_lang: description['name'][cur_lang] for cur_lang in description['name']})

desc = base.LocalizedString({cur_lang: description['description'][cur_lang] for cur_lang in description['description']})

task_spec = client.TaskSpec(
    id='MOS',
    function=function,
    name=name,
    description=desc,
    instruction=instruction)

task_duration_hint_9 = timedelta(seconds=9)

task_spec_prepared = client.PreparedTaskSpec(task_spec, lang)


def get_tasks(assignment_solutions):
    for assignment, solutions in assignment_solutions:
        for ((audio,), (score,)) in solutions:
            yield {
                'audio': audio.url,
                'label': int(score.value),
                'worker': assignment.user_id,
            }


def build_matrix(tasks):
    workers = list(set([item['worker'] for item in tasks]))

    sentences = list(set([item['audio'] for item in tasks]))

    matrix = np.zeros((len(workers), len(sentences)))
    matrix[:] = np.nan

    for task in tasks:
        worker = task['worker']
        sentence = task['audio']
        score = int(task['label'])
        matrix[workers.index(worker)][sentences.index(sentence)] = score

    return workers, sentences, matrix


def mos_variance(Z):
    (N, M) = Z.shape
    W = np.logical_not(np.isnan(Z))
    Mi = np.sum(W, 0)  # Mi = Mi(:)
    Nj = np.sum(W, 1)  # Nj = Nj(:)
    T = np.sum(W)

    v_su = vertical_var(Z.T)  # v_su  = v_s + v_u
    v_wu = vertical_var(Z)  # v_wu  = v_w + v_u
    v_swu = np.nanvar(Z)  # v_swu = v_s + v_w + v_u

    if not np.isnan(v_su) and not np.isnan(v_wu):
        v = np.linalg.solve([[1, 0, 1], [0, 1, 1], [1, 1, 1]], [v_su, v_wu, v_swu])  # v = [v_s ; v_w ; v_u]
        v = np.maximum(v, 0)

        v_s = v[0]
        v_w = v[1]
        v_u = v[2]

        v_mu = v_s * np.sum(Mi ** 2) / T ** 2 + v_w * np.sum(Nj ** 2) / T ** 2 + v_u / T
    elif np.isnan(v_su) and not np.isnan(v_wu):
        v = np.linalg.solve([[0, 1], [1, 1]], [v_wu, v_swu])  # v = [v_s ; v_wu]
        v = np.maximum(v, 0)

        v_s = v[0]
        v_wu = v[1]

        v_mu = v_s * np.sum(Mi ** 2) / T ** 2 + v_wu / T
    elif not np.isnan(v_su) and np.isnan(v_wu):
        v = np.linalg.solve([[0, 1], [1, 1]], [v_su, v_swu])  # v = [v_w ; v_su]
        v = np.maximum(v, 0)
        v_w = v[0]
        v_su = v[1]

        v_mu = v_w * np.sum(Nj ** 2) / T ** 2 + v_su / T
    else:
        assert (np.isnan(v_su) and np.isnan(v_wu))
        v_mu = v_swu / T
    return v_mu


def vertical_var(Z):
    rows, cols = Z.shape
    v = []
    for i in range(cols):
        if nancount(Z[:, i]) >= 2:
            v.append(np.nanvar(Z[:, i]))
    return np.mean(v)


def nancount(vec):
    return np.sum(np.logical_not(np.isnan(vec)))

class MOSEvaluationStrategy(evaluation.AssignmentAccuracyEvaluationStrategy):
    def __init__(self, submitted_assignments):
        self.workers, self.sentences, self.matrix = build_matrix(list(get_tasks(submitted_assignments)))
        self.mu = np.nanmean(self.matrix)  # MOS for each sentence
        self.s = np.nanstd(self.matrix)  # std dev for each sentence
        self.mu_norm = np.abs(self.matrix - self.mu) / self.s  # normalized scores

        self.outlying_scores = (self.matrix > 3.0)
        self.outlying_workers = np.sum(self.outlying_scores, 1) > 0.05 * np.sum(np.logical_not(np.isnan(self.matrix)),
                                                                                1)

    def evaluate_assignment(self, assignment_solutions: mapping.AssignmentSolutions) -> evaluation.AssignmentEvaluation:
        assignment, solutions = assignment_solutions
        ok_checks = int(self.outlying_workers[self.workers.index(assignment.user_id)])

        return evaluation.AssignmentEvaluation(
            assignment=assignment,
            ok_checks=ok_checks,
            total_checks=1,
            objects=len(solutions),
            incorrect_solution_indexes=[],
        )


def compute_mos_ci95_3gaussian(matrix):
    t1 = t.ppf(0.5 * (1 + 0.95), min(matrix.shape) - 1)  # t = 1.96
    mos = np.nanmean(matrix)
    ci95 = t1 * np.sqrt(mos_variance(matrix))

    return {"mu": round(mos, 2), "CI": round(ci95, 2)}


class MOSLoop(classification_loop.ClassificationLoop):
    def create_pool(
        self,
        control_objects,
        pool_cfg,
        check_project: bool = True,  # todo tmp for old pipelines
    ) -> toloka.Pool:
        if check_project:
            assert mapping.check_project_is_suitable(self.client.get_project(pool_cfg.project_id), self.task_mapping)
        assert pool_cfg.overlap == self.params.overlap.min_overlap
        pool = pool_config.create_pool_params(pool_cfg)
        lang_to_default_regions = description['default_regions']
        regions = lang_to_default_regions.get(self.lang, [])
        filter_regions = toloka.filter.FilterOr([toloka.filter.RegionByPhone.in_(code) for code in regions])


        #TODO: support other languages if needed
        if self.lang == 'EN':
            pool.filter = toloka.filter.FilterAnd([
                pool.filter,
                filter_regions,
                toloka.filter.Languages.not_in('RU'),
            ])

        # MOS: auto-accept period from instructions - 72h
        pool.auto_accept_period_day = 3

        pool = self.client.create_pool(pool)
        control_tasks = []
        for objects in control_objects:
            task = self.task_mapping.to_control_task(objects)
            task.pool_id = pool.id
            control_tasks.append(task)

        # MOS : no control tasks
        if control_tasks:
            logger.debug(f'creating {len(control_tasks)} control tasks')
            self.client.create_tasks(control_tasks, allow_defaults=True, async_mode=True, skip_invalid_items=False)
        return pool

    def get_task_id_to_overlap_increase(self, pool_id: str):
        return {}

    def loop(self, pool_id: str):
        iteration = 1
        while True:
            logger.debug(f'classification loop iteration #{iteration} is started')
            utils.wait_pool_for_close(self.client, pool_id)

            # TODO: collect stats about how workers answers control tasks and ignore bad control tasks by percentile

            submitted_assignments = self.get_assignments_solutions(
                pool_id, toloka.Assignment.SUBMITTED, with_control_tasks=True
            )

            logger.debug(f'{len(submitted_assignments)} submitted assignments are received')

            filtered_assignments = evaluation.prior_filter_assignments(
                self.client, submitted_assignments, self.params.control, self.lang
            )

            # this is MOS functionality
            # submitted - intentionally
            self.assignment_evaluation_strategy = MOSEvaluationStrategy(submitted_assignments)
            #

            logger.debug(f'{len(filtered_assignments)} assignments left after prior filter')

            evaluation.evaluate_submitted_assignments_and_apply_rules(
                filtered_assignments,
                self.assignment_evaluation_strategy,
                self.params.control,
                self.client,
                self.lang,
                pool_id,
            )

            tasks_to_rework = classification_loop.rework_not_finalized_tasks(
                self.client,
                self.get_assignments_solutions(pool_id, [toloka.Assignment.ACCEPTED, toloka.Assignment.REJECTED]),
                self.task_mapping,
                # possible cases:
                # I. static overlap - increase until needed min_overlap is reached
                # II. dynamic overlap -
                #   1. increase until needed min_overlap is reached
                #   2. increase after new accepted solution, because not enough confidence accumulated
                #   3. NOT increase after new accepted solution, because enough confidence accumulated already
                #   4. increase after new rejected solution, because not enough confidence accumulated
                #   5. NOT increase after new rejected solution, because enough confidence accumulated due to
                #        confidence recalculation because worker weights are also recalculated
                self.get_task_id_to_overlap_increase(pool_id),
                pool_id,
            )
            if tasks_to_rework == 0:
                return

            iteration += 1


def launch_mos(
    task_spec: PreparedTaskSpec,
    params: LoopParams,
    input_objects,
    control_objects,
    client: toloka.TolokaClient,
    interactive: bool = False,
):
    assert task_spec.scenario == project.Scenario.DEFAULT, 'You should use this function for crowd markup only'
    assert isinstance(params.task_duration_hint, timedelta)
    prj = check_project(task_spec, client)
    # MOS: no control tasks
    #     if not validate_objects_volume(
    #         input_objects,
    #         control_objects,
    #         params.task_duration_hint,
    #         params.pricing_config,
    #         interactive,
    #     ):
    #         return [], None

    if type(task_spec.function) not in (base.ClassificationFunction, base.SbSFunction):
        raise ValueError(f'unsupported task function: {task_spec.function}')

    # todo: check this some other way - assert isinstance(params, ClassificationLaunchParams)

    if interactive:
        pool_price = get_pool_price(len(input_objects), params.pricing_config, params.overlap, display_formula=True)
        if not ask(f'run classification of {len(input_objects)} objects for {pool_price}?'):
            return [], None
    # mos loop here
    loop = MOSLoop(
        client, task_spec.task_mapping, params.classification_loop_params, task_spec.lang
    )
    real_tasks_count = params.real_tasks_count
    control_tasks_count = params.control_tasks_count
    training_requirement = find_training_requirement(prj.id, task_spec, client, params)
    pool_cfg = pool_config.ClassificationConfig(
        project_id=prj.id,
        private_name='pool',
        lang=task_spec.lang,
        reward_per_assignment=params.assignment_price,
        task_duration_hint=params.task_duration_hint,
        real_tasks_count=real_tasks_count,
        control_tasks_count=control_tasks_count,
        overlap=params.overlap.min_overlap,
        control_params=params.control,
        training_requirement=training_requirement,
    )

    pool = loop.create_pool(control_objects, pool_cfg)

    #     stop_event = threading.Event()
    #     plotter = metrics.ClassificationMetricsPlotter(
    #         toloka_client=client,
    #         stop_event=stop_event,
    #         task_mapping=task_spec.task_mapping,
    #         pool_id=pool.id,
    #         task_duration_hint=params.task_duration_hint,
    #         params=params.classification_loop_params,
    #     )

    try:
        loop.add_input_objects(pool.id, input_objects)
        logger.info('classification has started')
        loop.loop(pool.id)
    except Exception as e:

        print(f'received exception: {e}, stopping loop')

        client.close_pool(pool.id)
        logger.info('classification is canceled')
        logger.debug(f'pool {pool.id} is closed')
        return [], None

    #     stop_event.set()
    #     plotter.join()

    return loop.get_results(pool.id, input_objects)

pricing_config = pricing.PoolPricingConfig(
    assignment_price=0.02,
    real_tasks_count=10,
    control_tasks_count=0,
)

params = classification_loop.Params(
    overlap=classification_loop.StaticOverlap(3),
    aggregation_algorithm=classification.AggregationAlgorithm.MAJORITY_VOTE,
    control=control.Control(
        rules=[
            control.Rule(
                predicate=control.TaskDurationPredicate(threshold=0.1, comparison=control.ComparisonType.LESS_OR_EQUAL, task_duration_hint=task_duration_hint_9),
                action=control.BlockUser(
                    scope=UserRestriction.Scope.POOL,
                    private_comment='Fast submits, <= 0.1',
                    duration=timedelta(days=2),
                ),
            ),
            control.Rule(
                predicate=control.TaskDurationPredicate(threshold=0.1, comparison=control.ComparisonType.LESS_OR_EQUAL, task_duration_hint=task_duration_hint_9),
                action=control.SetAssignmentStatus(status=toloka.Assignment.Status.REJECTED),
            ),
            control.Rule(
                predicate=control.AssignmentAccuracyPredicate(threshold=0.5, comparison=control.ComparisonType.GREATER_OR_EQUAL),
                action=control.SetAssignmentStatus(status=toloka.Assignment.ACCEPTED),
            ),
            control.Rule(
                predicate=control.AssignmentAccuracyPredicate(threshold=0.5, comparison=control.ComparisonType.LESS),
                action=control.SetAssignmentStatus(status=toloka.Assignment.REJECTED),
            ),
        ],
    )
)

loop_params = client.LoopParams(
    task_duration_hint=task_duration_hint_9,
    pricing_config=pricing_config,
    params=params,
    training_passing_skill_value=None,
)

client.define_task(task_spec_prepared, toloka_client)


raw_results, worker_weights = launch_mos(
    task_spec_prepared,
    loop_params,
    syn_audios,
    [],
    toloka_client,
    False,
)

from typing import List, Tuple, Dict, Optional
from enum import Enum


class MissingValue(Enum):
    MISSING = '-'


class FixedClassificationResults(client.ClassificationResults):
    def __init__(
        self,
        input_objects: List[mapping.Objects],
        results: List[Tuple[classification.TaskLabelsProbas, List[classification.WorkerLabel]]],
        task_spec: PreparedTaskSpec,
        worker_weights: Optional[Dict[str, float]] = None,
    ):
        self.task_spec = task_spec

        # we will index these input objects by dataframe row indexes
        row_input_objects = []

        input_fields = []
        for obj_mapping in task_spec.task_mapping.input_mapping:
            for _, task_field in obj_mapping.obj_task_fields:
                input_fields.append(task_field)

        self.input_fields = input_fields

        cls_type: tp.Type[base.Class] = task_spec.task_mapping.output_mapping[0].obj_cls
        cls_fields = [(cls, f'proba_{cls.value}') for cls in cls_type.possible_instances()]

        self.proba_fields = [cls_field for _, cls_field in cls_fields]
        self.has_worker_weights = worker_weights is not None
        rows = []
        for task_input_objects, result in zip(input_objects, results):
            task_row = task_spec.task_mapping.toloka_values(task_input_objects)
            del task_row[mapping.TASK_ID_FIELD]

            labels_probas, raw_labels = result

            tmp_result = classification.get_most_probable_label(labels_probas)
            if tmp_result is None:
                assert labels_probas is None
                result, proba, labels_probas = MissingValue.MISSING, {}, {}
                print(f'MISSING FOR: {task_input_objects}')
            else:
                result, proba = tmp_result
            task_row[self.RESULT_FIELD] = result.value
            task_row[self.CONFIDENCE_FIELD] = proba
            task_row[self.OVERLAP_FIELD] = len(raw_labels)
            for cls, cls_field in cls_fields:
                proba = labels_probas.get(cls, 0.0)
                task_row[cls_field] = proba

            for raw_label, worker in raw_labels:
                worker_row = dict(task_row)
                worker_row[self.LABEL_FIELD] = raw_label.value
                worker_row[self.WORKER_FIELD] = worker.id
                if self.has_worker_weights:
                    worker_row[self.WORKER_WEIGHT_FIELD] = worker_weights[worker.id]
                rows.append(worker_row)
                row_input_objects.append(task_input_objects)

        self.df = pd.DataFrame.from_dict(rows)
        self.row_input_objects = np.array(row_input_objects)


results = FixedClassificationResults(syn_audios, raw_results, task_spec_prepared, worker_weights)

# results = client.ClassificationResults(syn_audios, raw_results, task_spec_prepared, worker_weights)

with open(inputs.get('tasks'), "r") as read_file:
    tasks = json.load(read_file)

link_to_speaker = {}
link_to_synthesis_model = {}

for task in tasks:
    link_to_speaker[task['audio']] = task['speaker']
    link_to_synthesis_model[task['audio']] = task['synthesis_model']

predict = results.predict()
predict_proba = results.predict_proba()
worker_labels = results.worker_labels()

predict['speaker'] = predict['audio'].map(link_to_speaker)
predict_proba['speaker'] = predict_proba['audio'].map(link_to_speaker)
worker_labels['speaker'] = worker_labels['audio'].map(link_to_speaker)

predict['synthesis_model'] = predict['audio'].map(link_to_synthesis_model)
predict_proba['synthesis_model'] = predict_proba['audio'].map(link_to_synthesis_model)
worker_labels['synthesis_model'] = worker_labels['audio'].map(link_to_synthesis_model)

predict.to_csv(outputs.get('predict'))
predict_proba.to_csv(outputs.get('predict_proba'))
worker_labels.to_csv(outputs.get('worker_labels'))

unique_speakers = predict['speaker'].unique()

res = {}

for speaker in unique_speakers:
    res[speaker] = {}

for speaker in unique_speakers:
    for synthesis_model in ['prod', 'custom', 'speaker']:
        cur_worker_labels = worker_labels[(worker_labels['speaker'] == speaker) & (worker_labels['synthesis_model'] == synthesis_model)]

        if len(cur_worker_labels.index) != 0:
            cur_matrix = build_matrix(cur_worker_labels.to_dict(orient='records'))[-1]

            ci = compute_mos_ci95_3gaussian(cur_matrix)

            res[speaker][synthesis_model] = {"value": ci['mu'], "CI": [ci['mu'] - ci['CI'], ci['mu'] + ci['CI']]}

with open(outputs.get('metrics'), 'w', encoding='utf-8') as f:
    json.dump(res, f, ensure_ascii=False, indent=4)

shutil.copy('./dataforge.log', outputs.get('dataforge.log'))

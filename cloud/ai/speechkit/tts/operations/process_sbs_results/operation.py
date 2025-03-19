from collections import defaultdict
import json

from scipy import stats as st

from nope import JobProcessorOperation
from nope.endpoints import JSONInput, JSONOutput


class ProcessSbSResults(JobProcessorOperation):
    name = 'ProcessSbSResults'
    owner = 'cloud-ml'
    description = 'Process SbS results'

    class Inputs(JobProcessorOperation.Inputs):
        tasks = JSONInput(nirvana_name='tasks')
        left = JSONInput(nirvana_name='left')
        right = JSONInput(nirvana_name='right')

    class Outputs(JobProcessorOperation.Outputs):
        metrics = JSONOutput(nirvana_name='metrics')
        score = JSONOutput(nirvana_name='score')
        by_user = JSONOutput(nirvana_name='by_user')
        by_text = JSONOutput(nirvana_name='by_text')

    class Parameters(JobProcessorOperation.Parameters):
        pass

    def run(self):
        uploads_left = json.load(open(self.Inputs.left[0].get_path()))
        uploads_left = {x['url']: {'uuid': x['uuid'], 'text_uuid': x['text_uuid']} for x in uploads_left}
        uploads_right = json.load(open(self.Inputs.right[0].get_path()))
        uploads_right = {x['url']: {'uuid': x['uuid'], 'text_uuid': x['text_uuid']} for x in uploads_right}
        toloka_tasks = json.load(open(self.Inputs.tasks[0].get_path()))

        total_score = {'left': 0, 'right': 0}
        users_scores = defaultdict(lambda: {'left': 0, 'right': 0})
        texts_scores = defaultdict(lambda: {'left': 0, 'right': 0})

        for item in toloka_tasks:
            user = item['workerId']
            winner_url = item['outputValues']['result']
            if winner_url in uploads_left:
                text_uuid = uploads_left[winner_url]['text_uuid']
                winner = 'left'
            elif winner_url in uploads_right:
                text_uuid = uploads_right[winner_url]['text_uuid']
                winner = 'right'
            else:
                raise ValueError('Fail to match input files with toloka results.')
            total_score[winner] += 1
            users_scores[user][winner] += 1
            texts_scores[text_uuid][winner] += 1

        users_scores = list(map(lambda x: dict([('user_id', x[0])] + list(x[1].items())), users_scores.items()))
        texts_scores = list(map(lambda x: dict([('text_uuid', x[0])] + list(x[1].items())), texts_scores.items()))

        left_wins = total_score['left']
        right_wins = total_score['right']
        test_size = left_wins + right_wins

        metrics = {
            'test_size': test_size, 'right_wins_part': right_wins / test_size,
            'Wilcoxon_group_by_user': st.wilcoxon(list(map(lambda x: x['left'], users_scores)),
                                                  list(map(lambda x: x['right'], users_scores)))[1],
            'Wilcoxon_group_by_text': st.wilcoxon(list(map(lambda x: x['left'], texts_scores)),
                                                  list(map(lambda x: x['right'], texts_scores)))[1],
            'pvalue': st.binom_test(right_wins, test_size, alternative='greater')
        }

        json.dump(metrics, open(self.Outputs.metrics.get_path(), 'w'), ensure_ascii=False, indent=4)
        json.dump(total_score, open(self.Outputs.score.get_path(), 'w'), ensure_ascii=False, indent=4)
        json.dump(users_scores, open(self.Outputs.by_user.get_path(), 'w'), ensure_ascii=False, indent=4)
        json.dump(texts_scores, open(self.Outputs.by_text.get_path(), 'w'), ensure_ascii=False, indent=4)

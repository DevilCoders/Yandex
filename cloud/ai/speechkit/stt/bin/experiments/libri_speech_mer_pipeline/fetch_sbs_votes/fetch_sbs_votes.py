import json
from random import choice

import yt.wrapper as yt

from cloud.ai.lib.python.datetime import parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import SbSChoice
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.experiments.libri_speech_mer_pipeline import (
    MarkupSbS, table_markups_sbs_meta, table_name
)

no_noise_level_idx = -1


def main():
    yt.config['proxy']['url'] = 'hahn'

    table_markups = Table(meta=table_markups_sbs_meta, name=table_name)
    markups = []
    for row in yt.read_table(table_markups.path):
        markups.append(MarkupSbS.from_yson(row))

    # before 2021-01-24 markup is biased, because triples (hyp1, hyp2, ref) than one of hypothesis
    # is equal to reference are not considered
    markups = [m for m in markups if m.received_at > parse_datetime('2021-01-24T00:00:00.123456+00:00')]

    # for each descriptor (dataset, record, model, (noise_idx_1, noise_idx_2)) gather
    # - (less_noised_votes, more_noised_votes) for different noise levels, or
    # - (left_votes, right_votes) for equal noise levels

    desc_to_votes = {}
    for markup in markups:
        less_noise_idx = markup.noise_left.level_idx if markup.noise_left is not None else no_noise_level_idx
        more_noise_idx = markup.noise_right.level_idx if markup.noise_right is not None else no_noise_level_idx

        if less_noise_idx > more_noise_idx:
            less_noise_idx, more_noise_idx = more_noise_idx, less_noise_idx

        desc = (markup.dataset, markup.record, markup.model_left, (less_noise_idx, more_noise_idx))

        if desc not in desc_to_votes:
            desc_to_votes[desc] = (0, 0)

        if less_noise_idx == more_noise_idx:
            left_votes, right_votes = desc_to_votes[desc]

            if markup.choice == SbSChoice.LEFT:
                left_votes += 1
            else:
                right_votes += 1

            desc_to_votes[desc] = (left_votes, right_votes)
        else:
            less_noised_votes, more_noised_votes = desc_to_votes[desc]

            voted_noise = markup.noise_left if markup.choice == SbSChoice.LEFT else markup.noise_right

            voted_level_idx = voted_noise.level_idx if voted_noise is not None else no_noise_level_idx

            if voted_level_idx == less_noise_idx:
                less_noised_votes += 1
            else:
                more_noised_votes += 1

            desc_to_votes[desc] = (less_noised_votes, more_noised_votes)

    # calculate votes for descriptors and add them to (noise_idx_1, noise_idx_2) votes list

    agg_desc_to_votes = {}
    for desc, votes in desc_to_votes.items():
        dataset, _, model, noise_idx_pair = desc

        agg_desc = dataset, model, noise_idx_pair

        if agg_desc not in agg_desc_to_votes:
            agg_desc_to_votes[agg_desc] = (0, 0)

        # for equal noise levels it's left and right votes
        less_noised_votes_total, more_noised_votes_total = agg_desc_to_votes[agg_desc]
        less_noised_votes, more_noised_votes = votes

        less_noise_idx, more_noise_idx = noise_idx_pair
        if less_noise_idx == more_noise_idx:
            # Exclude task interface bias (users tend to choose LEFT text in case of meaning equality)
            if choice((True, False)):
                less_noised_votes, more_noised_votes = more_noised_votes, less_noised_votes

        agg_desc_to_votes[agg_desc] = (
            less_noised_votes_total + less_noised_votes,
            more_noised_votes_total + more_noised_votes,
        )

    # 39 corresponds to 10%
    # start from -40 because it will lead to -1 which is no noise idx
    noise_pairs_of_interest = [tuple(sorted((39, j))) for j in [39 + i for i in range(-40, 59 + 1, 10)]]

    result = []
    for agg_desc, votes in agg_desc_to_votes.items():
        dataset, model, noise_idx_pair = agg_desc

        if noise_idx_pair not in noise_pairs_of_interest:
            continue

        less_noise_idx, more_noise_idx = noise_idx_pair

        less_noised_votes, more_noised_votes = votes

        result.append({
            'dataset': dataset,
            'model': model,
            'less_noise_idx': less_noise_idx if less_noise_idx != no_noise_level_idx else None,
            'more_noise_idx': more_noise_idx if more_noise_idx != no_noise_level_idx else None,
            'less_noise_votes': less_noised_votes,
            'more_noise_votes': more_noised_votes,
        })

    with open('votes_sbs.json', 'w') as f:
        json.dump(result, f, indent=4, ensure_ascii=False)

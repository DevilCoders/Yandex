import json

import yt.wrapper as yt

from cloud.ai.speechkit.stt.lib.data.model.dao import SbSChoice
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.experiments.libri_speech_mer_pipeline import (
    MarkupSbS, NoiseData, table_markups_sbs_meta, table_name
)


def main():
    yt.config['proxy']['url'] = 'hahn'

    table_markups = Table(meta=table_markups_sbs_meta, name=table_name)
    markups = []
    for row in yt.read_table(table_markups.path):
        markups.append(MarkupSbS.from_yson(row))

    # for each descriptor (dataset, record, model, (noise_idx_1, noise_idx_2)) gather
    # - (less_noised_votes, more_noised_votes) for different noise levels, or
    # - (left_votes, right_votes) for equal noise levels

    desc_to_votes = {}

    for markup in markups:
        noise_level_left, noise_variant_left = get_noise_level_and_variant(markup.noise_left)
        noise_level_right, noise_variant_right = get_noise_level_and_variant(markup.noise_right)

        desc = (
            markup.reference,
            markup.hypothesis_left,
            markup.hypothesis_right,
            noise_level_left,
            noise_level_right,
            noise_variant_left,
            noise_variant_right,
            markup.model_left,
            markup.dataset,
        )

        if desc not in desc_to_votes:
            desc_to_votes[desc] = (0, 0)

        left_votes, right_votes = desc_to_votes[desc]

        if markup.choice == SbSChoice.LEFT:
            left_votes += 1
        else:
            right_votes += 1

        desc_to_votes[desc] = (left_votes, right_votes)

    result = []
    for desc, votes in desc_to_votes.items():
        reference, \
        hypothesis_left, \
        hypothesis_right, \
        noise_level_left, \
        noise_level_right, \
        noise_variant_left, \
        noise_variant_right, \
        model, \
        dataset = desc

        left_votes, right_votes = votes

        result.append({
            'reference': reference,
            'model': model,
            'dataset': dataset,
            'hypothesis_left': hypothesis_left,
            'hypothesis_right': hypothesis_right,
            'noise_level_left': noise_level_left,
            'noise_level_right': noise_level_right,
            'noise_variant_left': noise_variant_left,
            'noise_variant_right': noise_variant_right,
            'left_votes': left_votes,
            'right_votes': right_votes,
        })

    with open('texts.json', 'w') as f:
        json.dump(result, f, indent=4, ensure_ascii=False)


def get_noise_level_and_variant(noise: NoiseData):
    if noise is None:
        return 0.0, None
    return noise.level, noise.variant

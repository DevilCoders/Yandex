import json

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import parse_datetime
from cloud.ai.speechkit.stt.lib.experiments.libri_speech_mer_pipeline import (
    table_markups_check_meta, table_name, MarkupCheck
)


def main():
    yt.config['proxy']['url'] = 'hahn'

    table_markups = Table(meta=table_markups_check_meta, name=table_name)
    markups = []
    for row in yt.read_table(table_markups.path):
        markups.append(MarkupCheck.from_yson(row))

    # before 2021-01-24 markup is biased, because pairs (hyp, ref) than they are equal are not considered
    markups = [m for m in markups if m.received_at > parse_datetime('2021-01-24T00:00:00.123456+00:00')]

    desc_to_votes = {}
    for markup in markups:
        desc = (
            markup.recognition_id,
            markup.dataset,
            markup.record,
            markup.model,
            markup.noise.level_idx if markup.noise is not None else None,
        )

        if desc not in desc_to_votes:
            desc_to_votes[desc] = (0, 0)

        votes_total, votes_ok = desc_to_votes[desc]

        votes_total += 1
        if markup.ok:
            votes_ok += 1

        desc_to_votes[desc] = (votes_total, votes_ok)

    agg_desc_to_majorities = {}
    for desc, votes in desc_to_votes.items():
        _, dataset, _, model, level_idx = desc

        votes_total, votes_ok = votes
        # ok_win = False
        # if votes_ok > votes_total // 2:
        #     ok_win = True

        agg_desc = (dataset, model, level_idx)
        if agg_desc not in agg_desc_to_majorities:
            agg_desc_to_majorities[agg_desc] = (0, 0)

        acc_votes_total, acc_votes_ok = agg_desc_to_majorities[agg_desc]
        # acc_votes_total += 1
        # if ok_win:
        #     acc_votes_ok += 1

        agg_desc_to_majorities[agg_desc] = (acc_votes_total + votes_total, acc_votes_ok + votes_ok)

    result = []
    for agg_desc, majorities in agg_desc_to_majorities.items():
        dataset, model, level_idx = agg_desc
        votes_total, votes_ok = majorities
        result.append({
            'dataset': dataset,
            'model': model,
            'level_idx': level_idx,
            'votes_total': votes_total,
            'votes_ok': votes_ok,
        })

    with open('votes_check.json', 'w') as f:
        json.dump(result, f, indent=4, ensure_ascii=False)

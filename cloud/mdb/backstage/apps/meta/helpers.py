import json

import cloud.mdb.backstage.apps.meta.models as mod_models


MAX_SHOW_PILLAR_REVS = 50
PILLAR_REVS_LIMIT = 300


def get_pillar_revs_context(pillar_revs_limit=PILLAR_REVS_LIMIT, **kwargs):
    pillar_revs = mod_models.PillarRev.objects.filter(**kwargs)\
        .order_by('rev')\
        .values('rev', 'value')

    slice_from = pillar_revs.count() - pillar_revs_limit
    if slice_from < 0:
        slice_from = 0

    changes = []
    count = 0
    for change in mod_models.PillarRev.get_changes(pillar_revs[slice_from:]):
        if count < MAX_SHOW_PILLAR_REVS:
            break
        if change[1]:
            changes.append(change)
            count += 1

    return {'changes': changes}


def get_pillar_context(**kwargs):
    value = mod_models.Pillar.get_value(**kwargs)
    value = json.dumps(value, indent=4, sort_keys=True)
    lines = len(value)
    return {'value': value, 'lines': lines}

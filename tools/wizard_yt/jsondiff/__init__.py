#! /usr/bin/python
import yt.wrapper as yt

import jsondiff
import json
import sys


def remove_version(answer):
    for item in answer:
        if item.has_key('rules') and item['rules'].has_key('.version'):
            item['rules'].pop('.version', None)
    return answer


def full_diff(key, rows):
    a, b = rows.next(), rows.next()
    ans1 = remove_version(json.loads(a['begemot_answer']))
    ans2 = remove_version(json.loads(b['begemot_answer']))
    df = jsondiff.diff(ans1, ans2)
    text_diff = "".join(list(jsondiff.render_text([("wizard+Wizard", "WizardFull", df)]))).encode('utf-8')
    yield { "wizard+Wizard": ans1, "WizardFull": ans2, "diff": text_diff, "req": key['reqid'], "prepared_request": a["prepared_request"] }


@yt.reduce_aggregator
def short_diff(rows_groups):
    diffs = []
    for key, rows in rows_groups:
        a, b = rows.next(), rows.next()
        ans1 = remove_version(json.loads(a['begemot_answer']))
        ans2 = remove_version(json.loads(b['begemot_answer']))
        df = jsondiff.diff(ans1, ans2)
        diffs.append((key["reqid"], df))

    if len(diffs):
        grouped = jsondiff.group(diffs)
        text_diff = "".join(list(jsondiff.render_text((vs[0][0] + u' x{}'.format(len(vs)), vs[0][1]) for vs in grouped))).encode('utf-8')
        yield { "diff": text_diff  }

if len(sys.argv) < 4:
    sys.exit(1)

yt.config['pickling']['module_filter'] = lambda module: 'hashlib' not in getattr(module, '__name__', '')

if __name__ == "__main__":
    yt.run_reduce(
        short_diff,
        source_table=[sys.argv[1], sys.argv[2]],
        destination_table=sys.argv[3],
        local_files="jsondiff.py",
        reduce_by="reqid",
        spec={"max_failed_job_count": 0}
    )

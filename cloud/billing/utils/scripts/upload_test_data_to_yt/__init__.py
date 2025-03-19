def upload_test_data_to_yt(yt_cluster, yt_token, dst_table_path, data):
    import yt.wrapper as yt
    import json
    import time

    yt.config['token'] = yt_token
    yt.config['proxy']['url'] = yt_cluster
    tp = yt.TablePath(dst_table_path)
    ids = data.split()
    now = int(time.time())
    yt.write_table(
        tp, '\n'.join([
            json.dumps({
                'billing_account_id': id,
                'generated_at': now,
                'suspend_after': 1800
            }) for id in ids
        ]).encode("utf-8"), raw=True, format="json"
    )

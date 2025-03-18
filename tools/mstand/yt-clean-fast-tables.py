#!/usr/bin/env python3

import datetime as dt
import yt.wrapper as yt


def main():
    last_day_timestamp = (dt.datetime.now() - dt.timedelta(days=31)).timestamp()
    fast_tables_path = "//home/mstand/mstand_metrics/fast"

    result = yt.search(
        fast_tables_path,
        node_type=["table"],
        attributes=["resource_usage"],
        path_filter=lambda x: int(x.split("/")[-1]) < last_day_timestamp,
    )

    batch_client = yt.create_batch_client(raise_errors=True)
    disk_space = 0
    table_count = 0
    min_datetime = dt.datetime.max
    max_datetime = dt.datetime.min
    for path in result:
        table_count += 1
        disk_space += path.attributes["resource_usage"]["disk_space"]
        current_datetime = dt.datetime.fromtimestamp(int(path.split("/")[-1]))
        min_datetime = min(min_datetime, current_datetime)
        max_datetime = max(min_datetime, current_datetime)
        batch_client.remove(path)

    print("tables count:", table_count)
    print("disk space: %0.2f Gb" % (disk_space / 1024**3))
    print("min datetime:", min_datetime)
    print("max datetime:", max_datetime)

    batch_client.commit_batch()
    print("done")


if __name__ == "__main__":
    main()

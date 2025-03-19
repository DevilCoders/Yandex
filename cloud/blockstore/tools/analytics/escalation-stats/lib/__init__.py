from collections import defaultdict
import datetime
import json
import matplotlib.pyplot as plot
import os
import pathlib

import yt.wrapper as yt

LOG_PATH = "//statbox/juggler-banshee-log/"
DATE_FORMAT = "%Y-%m-%d"
MESSAGE_PREFIX = "Call to "
# 2022-02-09T16:08:13
TIMESTAMP_FORMAT = "%Y-%m-%dT%H:%M:%S"
CACHE_DIR = "./cache"


class WeeklyTotal:

    def __init__(self, d):
        self.date = d
        self.count = 0

    def __repr__(self):
        return "(%s, %s)" % (self.date.strftime(DATE_FORMAT), self.count)


class Stats:

    def __init__(self):
        self.by_person = defaultdict(int)
        self.by_check = defaultdict(int)
        self.by_date = defaultdict(int)
        self.total = 0
        self.weekly_totals = []


def cache_file_path(date_str):
    return os.path.join(CACHE_DIR, date_str)


def make_row_iterator(tab, date_str, ignore_cache, logger):
    f_path = cache_file_path(date_str)
    if os.path.exists(f_path) and not ignore_cache:
        with open(f_path) as f:
            for line in f.readlines():
                yield json.loads(line.rstrip())
    else:
        with open(f_path, "w") as f:
            processed = 0
            for row in yt.read_table(tab, format="json"):
                processed += 1
                if processed % 10000 == 0:
                    logger.debug("processed %s rows" % processed)

                if row.get("method") != "phone_escalation":
                    continue

                m = row.get("message", "")
                if not m.startswith(MESSAGE_PREFIX):
                    continue

                checks_str = row.get("checks", "")

                # TODO proper check filtration
                if "_nbs_" not in checks_str:
                    continue

                f.write(json.dumps(row))
                f.write("\n")

                yield row


def analyze_escalation_freq(
        from_date,
        to_date,
        plot_file_path,
        only_nighttime,
        proxy,
        pool,
        ignore_cache,
        logger):

    pathlib.Path(CACHE_DIR).mkdir(parents=True, exist_ok=True)

    yt.config["proxy"]["url"] = proxy

    f = datetime.datetime.strptime(from_date, DATE_FORMAT)
    t = datetime.datetime.strptime(to_date, DATE_FORMAT)

    overall_stats = Stats()
    nighttime_stats = Stats()

    while f <= t:
        f_str = f.strftime(DATE_FORMAT)
        tab = "%s%s" % (LOG_PATH, f_str)
        logger.info("processing table %s" % tab)

        def on_new_date(stats):
            if len(stats.weekly_totals) == 0 or (f - stats.weekly_totals[-1].date).days >= 7:
                stats.weekly_totals.append(WeeklyTotal(f))

        on_new_date(overall_stats)
        on_new_date(nighttime_stats)

        try:
            for row in make_row_iterator(tab, f_str, ignore_cache, logger):
                person = row.get("message", "")[len(MESSAGE_PREFIX):].split(" ")[0]

                checks = row.get("checks", "").split(",")

                ts = row.get("timestamp")
                is_nighttime = True
                if ts is not None:
                    tsd = datetime.datetime.strptime(ts, TIMESTAMP_FORMAT)
                    is_nighttime = tsd.hour > 0 and tsd.hour < 8
                else:
                    logger.error("no timestamp for row %s" % json.dumps(row))

                logger.debug("person: %s, checks: %s" % (person, checks))

                def update_stats(stats):
                    stats.by_person[person] += 1
                    for check in checks:
                        stats.by_check[check] += 1
                    stats.by_date[f_str] += 1
                    stats.total += 1

                    stats.weekly_totals[-1].count += 1

                update_stats(overall_stats)
                if is_nighttime:
                    update_stats(nighttime_stats)
        except Exception as e:
            logger.error("failed to process table %s, error: %s" % (tab, e))

        f += datetime.timedelta(days=1)

    def report_stats(by, label, sort_by_freq):
        lines = []
        for x, y in by.items():
            lines.append((":%s: %s %s" % (label, x, y), -y if sort_by_freq else x))

        lines.sort(key=lambda x: x[1])

        for line in lines:
            print(line[0])

    def make_plot(stats, color, label):
        print("%s weekly totals: %s" % (label, stats.weekly_totals))

        plot.plot(
            # [x.date.strftime(DATE_FORMAT) for x in stats.weekly_totals],
            [x.date for x in stats.weekly_totals],
            [x.count for x in stats.weekly_totals],
            color=color,
            label=label,
        )

    if not only_nighttime:
        report_stats(overall_stats.by_person, "person", True)
        report_stats(overall_stats.by_check, "check", True)
        report_stats(overall_stats.by_date, "date", False)
        print("TOTAL: %s" % overall_stats.total)
        make_plot(overall_stats, "blue", "overall")

    report_stats(nighttime_stats.by_person, "person (nighttime)", True)
    report_stats(nighttime_stats.by_check, "check (nighttime)", True)
    report_stats(nighttime_stats.by_date, "date (nighttime)", False)
    print("TOTAL (nighttime): %s" % nighttime_stats.total)
    make_plot(nighttime_stats, "red", "nighttime")

    plot.xticks(rotation=45)
    plot.savefig(plot_file_path, dpi=1000)

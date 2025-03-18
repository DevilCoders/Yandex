import csv
import logging
import sys
from collections import defaultdict

import adminka.adminka_helpers as admhelp
import experiment_pool.pool_helpers as phelp
import mstand_utils.testid_helpers as utestid
import mstand_utils.wiki_helpers as uwiki  # noqa
import reports.report_helpers as rhelp
import yaqutils.json_helpers as ujson
import yaqutils.template_helpers as utemplate
import yaqutils.time_helpers as utime
import yaqutils.six_helpers as usix
from adminka.pool_validation import PoolValidationErrors  # noqa
from experiment_pool import Experiment  # noqa
from experiment_pool import MetricColor
from experiment_pool import MetricResult  # noqa
from experiment_pool import Observation  # noqa
from experiment_pool import Pool  # noqa
from reports import ExperimentPair
from reports.report_helpers import format_num

import project_root


def get_ticket_queue(name):
    """
    :type name: str
    :return: str
    """
    return name.split("-")[0]


def get_ticket_name(obs, exp, session):
    obs_ticket = admhelp.fetch_observation_ticket(session, obs)
    testid_ticket = admhelp.fetch_abt_experiment_field(session, exp, "ticket")

    if obs_ticket is None and testid_ticket is None:
        return None
    if obs_ticket is None:
        return testid_ticket
    if testid_ticket is None:
        return obs_ticket

    if obs_ticket != testid_ticket:
        logging.warning("Observation %s has ticket %s, but experiment %s inside it has ticket %s",
                        obs, obs_ticket, exp, testid_ticket)

        if get_ticket_queue(obs_ticket) == "EXPERIMENTS" and get_ticket_queue(testid_ticket) != "EXPERIMENTS":
            logging.info("Choosing observation ticket {} over testid ticket {}".format(obs_ticket, testid_ticket))
            return obs_ticket

    return testid_ticket


def build_data_horizontal(exp_pairs, session, metric_number_map):
    """
    :type exp_pairs: list[ExperimentPair]
    :type session:
    :type metric_number_map:
    :rtype: list[dict]
    """

    rows = []
    for exp_pair in exp_pairs:
        obs = exp_pair.observation
        control = exp_pair.control
        exp = exp_pair.experiment

        abt_exp_title = admhelp.fetch_abt_experiment_field(session, exp, "title") or "unknown"

        if utestid.testid_is_simple(control.testid) and utestid.testid_is_simple(exp.testid):
            split_changes = session.get_split_change_info([control.testid, exp.testid], obs.dates)
        else:
            split_changes = {}
        row = {
            "observation": obs,
            "control": control,
            "experiment": exp,
            "abt_exp_title": abt_exp_title,
            "ticket": get_ticket_name(obs, exp, session),
            "metrics": [None] * len(metric_number_map),
            "split_changes": split_changes,
        }

        verdict = rhelp.get_verdict_by_testid(obs.tags, exp.testid)
        seen_colors = set()

        for metric_key, res_pair in usix.iteritems(exp_pair.result_pairs):
            one_result = {
                "exp_result": res_pair.exp_res,
                "color_by_value": MetricColor.YELLOW,
                "color_by_rows": MetricColor.YELLOW,
                # difference with horizontal mode
                "control_result": res_pair.control_res,
            }

            if res_pair.exp_res:
                # TODO: collect color_by_rows?
                seen_colors.add(res_pair.color_by_value)
                one_result["color_by_value"] = res_pair.color_by_value
                one_result["color_by_rows"] = res_pair.color_by_rows

            if res_pair.is_complete():
                one_result["diff"] = res_pair.get_diff()

            if verdict.is_bad() and res_pair.color_by_value == MetricColor.GREEN:
                one_result["verdict_discrepancy"] = True

            if verdict.is_good() and res_pair.color_by_value == MetricColor.RED:
                one_result["verdict_discrepancy"] = True

            metric_num = metric_number_map[metric_key]
            row["metrics"][metric_num] = one_result

        row["discrepancy"] = MetricColor.RED in seen_colors and MetricColor.GREEN in seen_colors
        row["same_color"] = len(seen_colors) == 1
        row["all_gray"] = seen_colors == {MetricColor.GRAY}
        rows.append(row)
    return rows


def collect_color_stats(exp_pairs, observations):
    """
    :type exp_pairs: list[ExperimentPair]
    :type observations: list[Observation]
    :rtype:
    """
    stats = defaultdict(lambda: {
        MetricColor.RED: 0,
        MetricColor.GRAY: 0,
        MetricColor.GREEN: 0,
        MetricColor.YELLOW: 0,
        "total": 0
    })

    for exp_pair in exp_pairs:
        for metric_key, res_pair in usix.iteritems(exp_pair.result_pairs):
            stats[metric_key][res_pair.color_by_value] += 1
            # TODO: collect color_by_rows?
            stats[metric_key]["total"] += 1

    # color sbs metrics as gray for now
    for obs in observations:
        if obs.sbs_metric_results:
            for sbs_metric_result in obs.sbs_metric_results:
                stats[sbs_metric_result.metric_key]["total"] += 1
                stats[sbs_metric_result.metric_key][MetricColor.GRAY] += 1

    return stats


def get_average(m_res):
    """
    :type m_res: MetricResult | None
    :rtype: float
    """
    if m_res is None:
        return 0
    return m_res.metric_values.average_val or 0


def build_tsv_rows(pool, session, threshold):
    """
    :type pool: Pool
    :type session:
    :type threshold: float
    :rtype:
    """
    header = [
        "Observation ID",
        "Test ID",
        "Control ID",
        "Name",
        "Ticket",
        "Start date",
        "End date",
        "Days"
    ]

    metric_number_map = phelp.enumerate_all_metrics(pool, sort_by_name=True)
    header.extend([mk.pretty_name() for mk in metric_number_map])
    yield header

    exp_pairs = ExperimentPair.from_pool(pool, fill_result_pairs=True)
    rhelp.colorize_experiment_pairs(exp_pairs, threshold)

    for row in build_data_horizontal(exp_pairs, session, metric_number_map):
        obs = row["observation"]
        exp = row["experiment"]
        control = row["control"]
        tsv_row = [
            obs.id,
            exp.testid,
            control.testid,
            row["abt_exp_title"],
            row["ticket"],
            utime.format_date(obs.dates.start),
            utime.format_date(obs.dates.end),
            obs.dates.number_of_days()
        ]

        for pair in row["metrics"]:
            if pair:
                row_str = build_one_tsv_row(pair)
            else:
                row_str = "N/A"
            tsv_row.append(row_str)

        yield tsv_row


def build_one_tsv_row(pair):
    """
    :type pair: dict
    :rtype: str
    """
    exp_result = pair["exp_result"]
    control_result = pair["control_result"]

    if exp_result and exp_result.criteria_results:
        pvalue = exp_result.criteria_results[0].pvalue
    else:
        pvalue = None

    # TODO handle sums properly
    if "diff" in pair:
        diff_absolute = pair["diff"].average.abs_diff
        diff_percent = pair["diff"].average.perc_diff_human
    else:
        diff_absolute = 0.0
        diff_percent = 0.0

    # FIXME: change to significant values
    exp_average = get_average(exp_result)
    control_average = get_average(control_result)

    return ("{exp_average} {control_average} {color}"
            " {pvalue} {diff_absolute} {diff_percent}").format(
        exp_average=format_num(exp_average),
        control_average=format_num(control_average),
        color=pair["color_by_value"],
        pvalue=format_num(pvalue, prec=3),
        diff_absolute=format_num(diff_absolute),
        diff_percent=format_num(diff_percent, prec=3)
    )


def write_tsv(fd, pool, session, threshold):
    """
    :type fd:
    :type pool: Pool
    :type session:
    :type threshold: float
    :return:
    """
    writer = csv.writer(fd, delimiter="\t")
    for row in build_tsv_rows(pool, session, threshold):
        writer.writerow(row)


class VerticalRow(object):
    def __init__(self,
                 observation,
                 control,
                 experiment,
                 abt_exp_title,
                 ticket,
                 sbs_ticket,
                 results,
                 discrepancy,
                 same_color,
                 all_gray,
                 is_first_in_obs,
                 split_changes,
                 ):
        """
        :type observation: Observation
        :type control: Experiment
        :type experiment: Experiment
        :type abt_exp_title: str
        :type ticket: str | None
        :type sbs_ticket: str | None
        :type results: list[VerticalResultPair]
        :type discrepancy: bool
        :type same_color: bool
        :type all_gray: bool
        :type is_first_in_obs: bool
        :type split_changes: dict
        """
        self.observation = observation
        self.control = control
        self.experiment = experiment
        self.abt_exp_title = abt_exp_title
        self.ticket = ticket
        self.sbs_ticket = sbs_ticket
        self.results = results
        self.discrepancy = discrepancy
        self.same_color = same_color
        self.all_gray = all_gray
        self.is_first_in_obs = is_first_in_obs
        self.split_changes = split_changes


def build_data_vertical(exp_pairs, session):
    """
    :type exp_pairs: list[ExperimentPair]
    :type session:
    :rtype: list[VerticalRow]
    """
    rows = []
    for exp_pair in exp_pairs:
        obs = exp_pair.observation
        exp = exp_pair.experiment
        control = exp_pair.control

        result_pairs, seen_colors = build_result_pairs_vertical(obs, exp_pair)
        abt_exp_title = admhelp.fetch_abt_experiment_field(session, exp, "title") or "unknown"

        ticket_name = get_ticket_name(obs, exp, session)

        if utestid.testid_is_simple(control.testid) and utestid.testid_is_simple(exp.testid):
            split_changes = session.get_split_change_info([control.testid, exp.testid], obs.dates)
        else:
            split_changes = {}

        discrepancy = MetricColor.RED in seen_colors and MetricColor.GREEN in seen_colors
        same_color = len(seen_colors) == 1
        all_gray = seen_colors == {MetricColor.GRAY}
        row = VerticalRow(
            observation=obs,
            control=control,
            experiment=exp,
            abt_exp_title=abt_exp_title,
            ticket=ticket_name,
            sbs_ticket=obs.sbs_ticket,
            results=result_pairs,
            discrepancy=discrepancy,
            same_color=same_color,
            all_gray=all_gray,
            is_first_in_obs=exp_pair.is_first_in_obs,
            split_changes=split_changes)
        rows.append(row)
    return rows


class VerticalResultPair(object):
    def __init__(self,
                 is_sbs_metric,
                 control_result,
                 exp_result,
                 sbs_result,
                 metric_key,
                 color_by_value,
                 color_by_rows,
                 criteria_results=None,
                 diff=None,
                 verdict_discrepancy=None,
                 is_first=None,
                 ):
        """
        For sbs metrics, fields control_result and exp_result store strings.

        :type is_sbs_metric: bool
        :type control_result: MetricResult | str | None
        :type exp_result: MetricResult | str | None
        :type sbs_result: str | None
        :type metric_key: user_plugins.plugin_key.PluginKey
        :type color_by_value: str
        :type color_by_rows: str
        :type criteria_results: list[CriteriaResult | None]
        :type diff: MetricDiff | None
        :type verdict_discrepancy: bool | None
        :type is_first: bool
        """
        self.is_sbs_metric = is_sbs_metric
        self.control_result = control_result
        self.exp_result = exp_result
        self.sbs_result = sbs_result
        self.metric_key = metric_key
        self.color_by_value = color_by_value
        self.color_by_rows = color_by_rows
        self.criteria_results = criteria_results
        self.diff = diff
        self.verdict_discrepancy = verdict_discrepancy
        self.is_first = is_first

    def get_control_result(self):
        """
        :rtype: str
        """
        if self.is_sbs_metric:
            return self.control_result
        else:
            return str(self.control_result.metric_values.significant_value)

    def get_exp_result(self):
        """
        :rtype: str
        """
        if self.is_sbs_metric:
            return self.exp_result
        else:
            return str(self.exp_result.metric_values.significant_value)

    def get_sbs_result(self):
        """
        :rtype: str
        """
        if self.is_sbs_metric:
            return self.sbs_result
        else:
            return ""


def build_result_pairs_vertical(obs, exp_pair):
    """
    :type obs: Observation
    :type exp_pair: ExperimentPair
    :rtype: list[VerticalResultPair]
    """
    result_pair_info = []
    verdict = rhelp.get_verdict_by_testid(obs.tags, exp_pair.experiment.testid)
    seen_colors = set()

    for metric_key, res_pair in usix.iteritems(exp_pair.result_pairs):

        result_pair = VerticalResultPair(
            is_sbs_metric=False,
            control_result=res_pair.control_res,
            exp_result=res_pair.exp_res,
            sbs_result=None,
            metric_key=metric_key,
            color_by_value=MetricColor.YELLOW,
            color_by_rows=MetricColor.YELLOW,
        )

        if res_pair.exp_res:
            # TODO: collect color_by_rows?
            seen_colors.add(res_pair.color_by_value)
            result_pair.color_by_value = res_pair.color_by_value
            result_pair.color_by_rows = res_pair.color_by_rows

            # difference with horizontal mode
            result_pair.criteria_results = res_pair.exp_res.criteria_results or [None]

        if res_pair.is_complete():
            result_pair.diff = res_pair.get_diff()

        if verdict.is_bad() and res_pair.color_by_value == MetricColor.GREEN:
            result_pair.verdict_discrepancy = True

        if verdict.is_good() and res_pair.color_by_value == MetricColor.RED:
            result_pair.verdict_discrepancy = True

        result_pair_info.append(result_pair)

    if exp_pair.control.sbs_system_id and exp_pair.experiment.sbs_system_id and obs.sbs_metric_results:
        control_sys_id = exp_pair.control.sbs_system_id
        exp_sys_id = exp_pair.experiment.sbs_system_id
        for sbs_metric_result in obs.sbs_metric_results:
            metric_key = sbs_metric_result.metric_key
            sbs_metric_values = sbs_metric_result.sbs_metric_values
            control_result = None
            experiment_result = None
            sbs_result = None
            if sbs_metric_values.single_results:
                for r in sbs_metric_values.single_results:
                    if r["sys_id"] == control_sys_id:
                        control_result = r["result"]
                    if r["sys_id"] == exp_sys_id:
                        experiment_result = r["result"]
            if sbs_metric_values.pair_results:
                for r in sbs_metric_values.pair_results:
                    if r["sys_id_left"] == exp_sys_id and r["sys_id_right"] == control_sys_id:
                        sbs_result = r["result"]

            result_pair = VerticalResultPair(
                is_sbs_metric=True,
                control_result=ujson.dump_to_str(control_result),
                exp_result=ujson.dump_to_str(experiment_result),
                sbs_result=ujson.dump_to_str(sbs_result),
                metric_key=metric_key,
                color_by_value=MetricColor.GRAY,
                color_by_rows=MetricColor.GRAY,
            )
            seen_colors.add(result_pair.color_by_value)
            result_pair_info.append(result_pair)

    result_pair_info.sort(key=lambda x: x.metric_key)
    for index, result_pair in enumerate(result_pair_info):
        result_pair.is_first = (index == 0)

    return result_pair_info, seen_colors


def get_fd(path):
    if path is not None:
        if path == "-":
            return sys.stdout
        else:
            return open(path, "w")
    return None


def write_markup(path, text):
    fd = get_fd(path)
    if fd:
        with fd as fd:
            fd.write(text)


def detect_red_lamps(exp_pairs):
    """
    :type exp_pairs: list[ExperimentPair]
    :rtype: bool
    """
    for exp_pair in exp_pairs:
        for res_pair in exp_pair.result_pairs.values():
            if res_pair.exp_res:
                if res_pair.get_lamp_color() == MetricColor.RED:
                    return True
    return False


def create_template_environment(pool, session, validation_result, threshold, show_all_gray,
                                show_same_color, source_url="", template_dir=None):
    """
    :type pool: Pool
    :type session:
    :type validation_result: PoolValidationErrors | None
    :type threshold: float
    :type show_all_gray: bool
    :type show_same_color: bool
    :type source_url: str
    :type template_dir: str | None
    :return:
    """

    logging.info("create_template_environment started")
    if template_dir is None:
        template_dir = project_root.get_project_path("templates")
    env = utemplate.get_environment(template_dir)

    logging.info("enumerate_all_metrics")
    metric_keys = phelp.enumerate_all_metrics(pool, sort_by_name=True)
    lamp_metric_keys = phelp.enumerate_all_metrics_in_lamps(pool, sort_by_name=True)

    exp_pairs = ExperimentPair.from_pool(pool, fill_result_pairs=True)
    lamp_exp_pairs = ExperimentPair.from_lamps(pool)
    rhelp.colorize_experiment_pairs(exp_pairs, threshold)
    rhelp.colorize_experiment_pairs(lamp_exp_pairs, threshold)

    logging.info("collect_stats")
    stats = collect_color_stats(exp_pairs, pool.observations)

    logging.info("build_data_horizontal")
    data_horizontal = build_data_horizontal(exp_pairs, session, metric_keys)

    logging.info("build_data_vertical")
    data_vertical = build_data_vertical(exp_pairs, session)
    red_lamps = detect_red_lamps(lamp_exp_pairs)

    lamp_stats = collect_color_stats(lamp_exp_pairs, pool.observations)
    lamp_data_horizontal = build_data_horizontal(lamp_exp_pairs, session, lamp_metric_keys)
    lamp_data_vertical = build_data_vertical(lamp_exp_pairs, session)

    logging.info("environment preparation done")

    if validation_result is None:
        validation_result = ""
    else:
        validation_result = validation_result.pretty_print(show_ok=False)

    env.globals = {
        "threshold": threshold,
        "metrics": metric_keys,
        "validation_errors": validation_result,
        "show_all_gray": show_all_gray,
        "show_same_color": show_same_color,
        "data_horizontal": data_horizontal,
        "data_vertical": data_vertical,
        "stats": stats,
        "lamp_data_horizontal": lamp_data_horizontal,
        "lamp_data_vertical": lamp_data_vertical,
        "lamp_stats": lamp_stats,
        "red_lamps": red_lamps,
        "source_url": source_url,
        "format_date": utime.format_date,
    }
    return env

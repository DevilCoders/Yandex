# -*- coding: utf-8 -*-

import base64
import itertools
import logging

import bokeh.layouts as bol
import bokeh.models as bmod
import bokeh.plotting as bplot

import experiment_pool.pool_helpers as pool_helpers
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import MetricColor, MetricColoring
from reports import MetricResultPair

BASE_WIDTH = 900
BASE_HEIGHT = 900

# control diamonds
SIZE_CONTROL = 20

COLOR_CONTROL = "#F44336"  # Material Red
ALPHA_CONTROL = 0.4
ALPHA_CONTROL_SELECTED = 1

# experiment circles
SIZE_EXPERIMENT = 15

COLOR_EXP_NONE_SIGNIFICANT = "#9E9E9E"  # Material Gray
ALPHA_EXP_NONE_SIGNIFICANT = 0.2

COLOR_EXP_FIRST_SIGNIFICANT = "#FF9800"  # Material Orange
COLOR_EXP_SECOND_SIGNIFICANT = "#673AB7"  # Material Deep Purple
ALPHA_EXP_ONE_SIGNIFICANT = 0.6

COLOR_EXP_BOTH_SIGNIFICANT = "#2196F3"  # Material Blue
ALPHA_EXP_BOTH_SIGNIFICANT = 0.9

COLOR_EXP_SELECTED = "#C6FF00"  # Material Lime
ALPHA_EXP_SELECTED = 1

# control <-> experiment lines
COLOR_LINE = "black"
ALPHA_LINE = 0.3

COLOR_LEGEND_BACKGROUND = "white"
ALPHA_LEGEND_BACKGROUND = 0.9


def _get_pvalue(exp_result):
    if exp_result.criteria_results:
        return exp_result.criteria_results[0].pvalue
    return -1


class MetricPairPlotter(object):
    def __init__(self, pool, metric_x, metric_y, threshold):
        """
        :type metric_x: PluginKey
        :type metric_y: PluginKey
        """
        self.exp_xs = []
        self.exp_ys = []

        self.control_xs = []
        self.control_ys = []

        self.exp_testids = []
        self.exp_control_testids = []

        self.control_only_testids = []

        self.line_xs = []
        self.line_ys = []

        self.diff_xs = []
        self.diff_ys = []

        self.pvalue_xs = []
        self.pvalue_ys = []

        self.colors = []
        self.alphas = []

        self.metric_x = metric_x
        self.metric_y = metric_y
        self.threshold = threshold

        self.flip_x = False
        self.flip_y = False

        for observation in pool.observations:
            self.add_observation(observation)

        self.exps_data_source = self.create_exps_data_source()
        self.controls_data_source = self.create_controls_data_source()

    def _get_metric_results(self, experiment):
        results = experiment.get_metric_results_map()
        return results.get(self.metric_x), results.get(self.metric_y)

    def add_observation(self, observation):
        logging.debug("    - collecting observation %s...", observation)
        result_x, result_y = self._get_metric_results(observation.control)

        if result_x is None:
            logging.debug("    - control doesn't have metric %s, skipping observation", self.metric_x)
            return

        if result_y is None:
            logging.debug("    - control doesn't have metric %s, skipping observation", self.metric_y)
            return

        if result_x.coloring == MetricColoring.LESS_IS_BETTER:
            self.flip_x = True

        if result_y.coloring == MetricColoring.LESS_IS_BETTER:
            self.flip_y = True

        self.add_control(observation.control)
        for experiment in observation.experiments:
            self.add_exp_pair(observation.control, experiment)

    def add_control(self, control):
        logging.debug("      - collecting control %s...", control)
        result_x, result_y = self._get_metric_results(control)

        control_x = result_x.metric_values.significant_value
        control_y = result_y.metric_values.significant_value

        self.control_xs.append(control_x)
        self.control_ys.append(control_y)

        self.control_only_testids.append(control.testid)

    def add_exp_pair(self, control, experiment):
        logging.debug("      - collecting experiment %s vs control %s...", experiment, control)

        control_result_x, control_result_y = self._get_metric_results(control)
        exp_result_x, exp_result_y = self._get_metric_results(experiment)

        if exp_result_x is None:
            logging.debug("    - experiment doesn't have metric %s, skipping observation", self.metric_x)
            return

        if exp_result_y is None:
            logging.debug("    - experiment doesn't have metric %s, skipping observation", self.metric_y)
            return

        self.exp_control_testids.append(control.testid)
        self.exp_testids.append(experiment.testid)

        exp_x = exp_result_x.metric_values.significant_value
        exp_y = exp_result_y.metric_values.significant_value

        self.exp_xs.append(exp_x)
        self.exp_ys.append(exp_y)

        self.line_xs.append((control_result_x.metric_values.significant_value, exp_x))
        self.line_ys.append((control_result_y.metric_values.significant_value, exp_y))

        pair_x = MetricResultPair(control_result_x, exp_result_x)
        pair_y = MetricResultPair(control_result_y, exp_result_y)

        pair_x.colorize(self.threshold, rows_threshold=None)
        pair_y.colorize(self.threshold, rows_threshold=None)

        diff_x = pair_x.get_diff()
        diff_y = pair_y.get_diff()

        self.diff_xs.append(diff_x.significant.abs_diff)
        self.diff_ys.append(diff_y.significant.abs_diff)

        self.pvalue_xs.append(_get_pvalue(exp_result_x))
        self.pvalue_ys.append(_get_pvalue(exp_result_y))

        self.add_colors(pair_x, pair_y)

    def add_colors(self, pair_x, pair_y):
        """
        :type pair_x: MetricResultPair
        :type pair_y: MetricResultPair
        :return:
        """
        is_sign_1 = pair_x.color_by_value != MetricColor.GRAY
        is_sign_2 = pair_y.color_by_value != MetricColor.GRAY
        if not is_sign_1 and not is_sign_2:
            self.alphas.append(ALPHA_EXP_NONE_SIGNIFICANT)
            self.colors.append(COLOR_EXP_NONE_SIGNIFICANT)
        elif is_sign_1 and not is_sign_2:
            self.alphas.append(ALPHA_EXP_ONE_SIGNIFICANT)
            self.colors.append(COLOR_EXP_FIRST_SIGNIFICANT)
        elif not is_sign_1 and is_sign_2:
            self.alphas.append(ALPHA_EXP_ONE_SIGNIFICANT)
            self.colors.append(COLOR_EXP_SECOND_SIGNIFICANT)
        else:
            self.alphas.append(ALPHA_EXP_BOTH_SIGNIFICANT)
            self.colors.append(COLOR_EXP_BOTH_SIGNIFICANT)

    def create_exps_data_source(self):
        return bplot.ColumnDataSource(
            data={
                "x": self.exp_xs,
                "y": self.exp_ys,
                "testid": self.exp_testids,
                "control": self.exp_control_testids,
                "diff_x": self.diff_xs,
                "diff_y": self.diff_ys,
                "pvalue_x": self.pvalue_xs,
                "pvalue_y": self.pvalue_ys,
                "colors": self.colors,
                "alphas": self.alphas
            }
        )

    def create_controls_data_source(self):
        zeros = [0] * len(self.control_xs)
        return bplot.ColumnDataSource(
            data={
                "x": self.control_xs,
                "y": self.control_ys,
                "testid": self.control_only_testids,
                "control": self.control_only_testids,
                "diff_x": zeros,
                "diff_y": zeros,
                "pvalue_x": zeros,
                "pvalue_y": zeros,
            }
        )

    def create_figure(self, label, flip_by_coloring=True):
        logging.debug("    - creating bokeh plot...")

        metric_name_x = self.metric_x.pretty_name()
        metric_name_y = self.metric_y.pretty_name()

        tap = bmod.TapTool()
        tap.callback = bmod.OpenURL(url="https://ab.yandex-team.ru/testid/@testid")

        hover = bmod.HoverTool(
            tooltips=[
                ("testid", "@testid"),
                ("control", "@control"),
                (metric_name_x, "@x"),
                ("- diff", "@diff_x"),
                ("- pvalue", "@pvalue_x"),
                (metric_name_y, "@y"),
                ("- diff", "@diff_y"),
                ("- pvalue", "@pvalue_y"),
            ]
        )

        figure = bplot.figure(
            title=label.format(x=metric_name_x, y=metric_name_y),
            tools=[
                # click tools
                tap,
                # drag tools
                "pan", "box_zoom", "box_select",
                # scroll tools
                "wheel_zoom",
                # buttons
                "undo", "redo", "reset", "save",
                # hover tooltips
                hover,
                # broken
                # click tools
                # "poly_select",
                # drag tools
                # "lasso_select",
            ],
            plot_width=BASE_WIDTH,
            plot_height=BASE_HEIGHT,
            active_tap=tap,
            active_drag="pan",
            active_scroll="wheel_zoom"
        )

        figure.xaxis.axis_label = metric_name_x
        figure.yaxis.axis_label = metric_name_y

        logging.debug("    - adding legends...")
        figure.circle([], [], fill_color=COLOR_EXP_NONE_SIGNIFICANT, line_color=None,
                      legend_label="both metrics are not significant")
        figure.circle([], [], fill_color=COLOR_EXP_FIRST_SIGNIFICANT, line_color=None,
                      legend_label="{} is significant, {} is not".format(metric_name_x, metric_name_y))
        figure.circle([], [], fill_color=COLOR_EXP_SECOND_SIGNIFICANT, line_color=None,
                      legend_label="{} is significant, {} is not".format(metric_name_y, metric_name_x))
        figure.circle([], [], fill_color=COLOR_EXP_BOTH_SIGNIFICANT, line_color=None,
                      legend_label="both metrics are significant")

        figure.legend.background_fill_color = COLOR_LEGEND_BACKGROUND
        figure.legend.background_fill_alpha = ALPHA_LEGEND_BACKGROUND

        if flip_by_coloring:
            if self.flip_x:
                figure.x_range = bmod.DataRange1d(flipped=True)
            if self.flip_y:
                figure.y_range = bmod.DataRange1d(flipped=True)

        return figure

    def draw_value_plot(self):
        logging.debug("  - value plot")
        figure = self.create_figure("{x} vs. {y}")

        logging.debug("    - plotting lines...")
        figure.multi_line(
            xs=self.line_xs,
            ys=self.line_ys,

            color=COLOR_LINE,
            alpha=ALPHA_LINE
        )

        logging.debug("    - plotting controls...")
        figure.diamond(
            source=self.controls_data_source,

            x="x",
            y="y",

            size=SIZE_CONTROL,
            fill_color=COLOR_CONTROL,
            fill_alpha=ALPHA_CONTROL,
            selection_fill_color=COLOR_CONTROL,
            selection_fill_alpha=ALPHA_CONTROL_SELECTED,
            line_color=None,

            legend_label="controls"
        )

        logging.debug("    - plotting experiments...")
        figure.circle(
            source=self.exps_data_source,

            x="x",
            y="y",

            size=SIZE_EXPERIMENT,
            fill_color="colors",
            fill_alpha="alphas",
            selection_fill_color=COLOR_EXP_SELECTED,
            selection_fill_alpha=ALPHA_EXP_SELECTED,
            line_color=None,
        )

        return figure

    def draw_diff_plot(self):
        logging.debug("  - diff plot")
        figure = self.create_figure("{x} vs. {y} - diff")

        logging.debug("    - plotting experiments...")
        figure.circle(
            source=self.exps_data_source,

            x="diff_x",
            y="diff_y",

            size=SIZE_EXPERIMENT,
            fill_color="colors",
            fill_alpha="alphas",
            selection_fill_color=COLOR_EXP_SELECTED,
            selection_fill_alpha=ALPHA_EXP_SELECTED,
            line_color=None
        )

        return figure

    def draw_pvalue_plot(self):
        logging.debug("  - pvalue plot")
        figure = self.create_figure("{x} vs. {y} - pvalue", flip_by_coloring=False)

        logging.debug("    - plotting experiments...")
        figure.circle(
            source=self.exps_data_source,

            x="pvalue_x",
            y="pvalue_y",

            size=SIZE_EXPERIMENT,
            fill_color="colors",
            fill_alpha="alphas",
            selection_fill_color=COLOR_EXP_SELECTED,
            selection_fill_alpha=ALPHA_EXP_SELECTED,
            line_color=None
        )

        return figure


def plot_metric_pair_html(pool, metric_x, metric_y, threshold):
    logging.info("plotting %s vs %s", metric_x.pretty_name(), metric_y.pretty_name())

    plotter = MetricPairPlotter(pool, metric_x, metric_y, threshold)
    value_plot = plotter.draw_value_plot()
    diff_plot = plotter.draw_diff_plot()
    pvalue_plot = plotter.draw_pvalue_plot()

    column = bol.column(value_plot, diff_plot, pvalue_plot)
    import bokeh.embed
    script, content = bokeh.embed.components(column)
    script = script.replace('document.readyState != "loading"', "true")

    html = """
            <head>
                <title>{metric_x} vs {metric_y}</title>
                <link
                    href="https://cdn.pydata.org/bokeh/release/bokeh-{bokeh_ver}.css"
                    rel="stylesheet"
                    type="text/css"
                >
                <link
                    href="https://cdn.pydata.org/bokeh/release/bokeh-widgets-{bokeh_ver}.css"
                    rel="stylesheet"
                    type="text/css"
                >
            </head>
            <body>
                {content}
            </body>
            """.format(
        metric_x=metric_x.pretty_name(),
        metric_y=metric_y.pretty_name(),
        content=content,
        bokeh_ver=bokeh.__version__
    )
    return html, script


def b64encode(text):
    return base64.b64encode(text.encode("utf8")).decode()


def plot_pool(pool, fd, threshold):
    import bokeh.embed

    metrics = pool_helpers.enumerate_all_metrics(pool)

    fd.write("""
    <html>
    <head>
        <title>mstand compare plot</title>
        <script>
            function addScript(doc, src, callback) {{
                var script = doc.createElement('script');
                script.type = 'text/javascript';
                script.src = src;
                script.async = false;
                if(callback) {{
                    script.addEventListener('load', function (e) {{ callback(); }}, false);
                }}
                doc.getElementsByTagName('head')[0].appendChild(script);
            }}

            function openWindow(content_b64, script_b64) {{
                var content = window.atob(content_b64);
                var wnd = window.open("about:blank", "_blank");
                wnd.document.write(content);

                function show() {{
                    var content = window.atob(script_b64);
                    wnd.document.write(content);
                }}

                addScript(wnd.document, "https://cdn.pydata.org/bokeh/release/bokeh-{bokeh_ver}.js", null);
                addScript(wnd.document, "https://cdn.pydata.org/bokeh/release/bokeh-widgets-{bokeh_ver}.js", show);
            }}
        </script>
    </head>
    <body>
    """.format(bokeh_ver=bokeh.__version__))

    num_plots = sum(range(len(metrics)))

    for idx, (metric_x, metric_y) in enumerate(itertools.combinations(metrics.keys(), 2)):
        umisc.log_progress("plotting graphs", idx, num_plots)
        html, script = plot_metric_pair_html(pool, metric_x, metric_y, threshold)
        fd.write("""<a href="#" onclick="openWindow('{}', '{}');">{} vs {}</a><br/>""".format(
            b64encode(html),
            b64encode(script),
            metric_x.pretty_name(),
            metric_y.pretty_name()
        ))

    fd.write("</body></html>")
    fd.close()

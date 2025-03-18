{% macro stats_table(stats) %}
    <table class="stats_table">
        <thead>
            <tr>
                <th>Metric</th>
                <th class="metric-green">Green</th>
                <th class="metric-yellow">Yellow</th>
                <th class="metric-red">Red</th>
                <th class="metric-gray">Gray</th>
                <th>Total</th>
            </tr>
        </thead>
        <tbody>
            {% for metric, stat_entry in stats | dictsort %}
                <tr>
                    <td>{{ metric.pretty_name() }}</td>
                    <td class="metric-green">{{ stat_entry.green }}</td>
                    <td class="metric-yellow">{{ stat_entry.yellow }}</td>
                    <td class="metric-red">{{ stat_entry.red }}</td>
                    <td class="metric-gray">{{ stat_entry.gray }}</td>
                    <td>{{ stat_entry.total }}</td>
                </tr>
            {%- endfor %}
        </tbody>
    </table>
{% endmacro %}

<!DOCTYPE html>
<html>
<head>
    <title>{% block title %}{% endblock %}</title>
    <meta charset="utf-8"/>

    <!--

    Нирвана очень любит угадывать кодировки. Давайте поможем Нирване угадать кодировку!

    Мой дядя как-то сайт фигачил.
    Чтоб тыкву не мороковать -
    Фрагмент "Онегина" вкорячил
    Чтоб кодировку угадать.
    -->

    <!--suppress CssUnusedSymbol -->
    <style>
        * {
            font-size: 14px;
            font-family: sans-serif;
        }

        table {
            border-collapse: collapse;
        }

        table td,
        table th {
            border: 1px solid #aaaaaa;
            padding: 5px;
            border-collapse: collapse;
        }

        table thead th {
            font-weight: bold;
            cursor: ns-resize;
        }

        .bold {
            font-weight: bold;
        }

        a {
            text-decoration: none;
        }

        .warning {
            color: #cc0000;
        }

        .hovertext {
            border-bottom: 1px dotted black;
        }

        .nodata {
            color: #aaaaaa;
            font-style: italic;
        }

        .result-block tr td:last-child {
            width: 100% !important;
        }

        .numeric {
            text-align: right;
            font-family: monospace;
            white-space: nowrap;
        }

        td.nested-container {
            padding: 0;
            height: 100%;
        }

        table.nested {
            width: 100%;
            height: 100%;
            border: none;
        }

        table.nested td {
            white-space: nowrap;
        }

        table.no-outer-border > tbody > tr:first-child > td {
            border-top: none;
        }

        table.no-outer-border > tbody > tr:last-child > td {
            border-bottom: none;
        }

        table.no-outer-border > tbody > tr > td:first-child {
            border-left: none;
        }

        table.no-outer-border > tbody > tr > td:last-child {
            border-right: none;
        }

        .metrics {
            margin-top: 10px;
        }

        .metric-red {
            color: red;
        }

        .metric-gray {
            color: #707070;
        }

        .metric-green {
            color: #009000;
        }

        .metric-yellow {
            color: goldenrod;
        }

        .hidden {
            display: none;
        }

        div.colorbox {
            height: 0.8em;
            width: 0.8em;
            display: inline-block;
        }

        .filter__text {
            width: 446px;
        }

        .filter__error {
            color: red;
        }

        .filter__help {
            position: relative;
            padding: 8px;
        }

        .filter__popup {
            display: none;
            position: absolute;
            left: -350px;
            top: 30px;
            width: 750px;
            padding: 0 10px;
            z-index: 5;
            background-color: #fff;
            border: 1px solid #aaa;
        }

        .filter__help:hover .filter__popup {
            display: block;
        }

        .filter__popup p {
            font: 12px/18px Consolas, monospace, sans-serif;
        }

        .filter__found {
            padding-right: 8px;
        }

        .metrics__row_hidden {
            display: none;
        }

        .metric__meta_additional > table,
        .metric__meta_additional > div {
            display: none;
        }

        .metrics_filtered .metric__meta_additional > table {
            display: table;
        }

        .metrics_filtered .metric__meta_additional > div {
            display: block;
        }

        .error-box {
            background-color: #f44336; /* Material red 500 */
            color: white;
        }

        .warning-box {
            background-color: #ffc107; /* Material amber 500 */
        }

        #warning-close {
            float: right;
            color: white;
            cursor: pointer;
        }

        .tag {
            font-size: 12px;
            line-height: 23px;
            text-decoration: none;

            display: inline-block;

            color: white;
            background: #3f51b5; /* Material indigo 500 */
            border-radius: 3px;

            padding: 0 5px 0 5px;
            margin: 0 5px 5px 0;

            cursor: pointer;
        }

        .example:hover {
            border-bottom: 1px dotted gray;
            cursor: help;
        }

        details summary {
            user-select: none;
            outline: none;
            cursor: pointer;
        }

        #lamps_table {
            font-size: 20px;
        }
    </style>
    {% block scripts %}{% endblock %}
</head>
<body>
{% block content %}{% endblock %}
</body>
</html>


import plotly.graph_objects as graph_objects

from plotly.subplots import make_subplots
from datetime import datetime

COLORS = ["green", "blue", "black", "blueviolet", "red", "cadetblue", "chocolate", "coral",
          "cornflowerblue", "crimson", "cyan", "darkblue", "darkcyan", "darkgoldenrod",
          "darkgreen", "darkkhaki", "darkmagenta", "darkolivegreen",
          "darkorange", "darkorchid", "darkred", "darksalmon", "darkseagreen",
          "darkslateblue", "darkslategray", "darkslategrey", "darkturquoise",
          "darkviolet", "deeppink", "deepskyblue", "dimgray", "dimgrey",
          "dodgerblue", "firebrick", "floralwhite", "forestgreen", "fuchsia",
          "gainsboro", "ghostwhite", "gold", "goldenrod", "gray", "grey",
          "honeydew", "hotpink", "indianred",
          "indigo", "ivory", "khaki", "lavender", "lavenderblush", "lawngreen",
          "lemonchiffon", "lightblue", "lightcoral", "lightcyan",
          "lightgray", "lightgrey", "lightgreen", "lightpink", "lightsalmon",
          "lightseagreen", "lightskyblue", "lightslategray", "lightslategrey",
          "lightsteelblue", "lime", "limegreen", "linen",
          "magenta", "maroon", "mediumaquamarine", "mediumblue", "mediumorchid",
          "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumspringgreen",
          "mediumturquoise", "mediumvioletred", "midnightblue", "mintcream",
          "mistyrose", "moccasin", "navajowhite", "navy", "oldlace", "olive",
          "olivedrab", "orange", "orangered", "orchid", "palegoldenrod",
          "palegreen", "paleturquoise", "palevioletred", "papayawhip", "peachpuff",
          "peru", "pink", "plum", "powderblue", "purple", "rosybrown",
          "royalblue", "rebeccapurple", "saddlebrown", "salmon", "sandybrown",
          "seagreen", "seashell", "sienna", "silver", "skyblue", "slateblue",
          "slategray", "slategrey", "snow", "springgreen", "steelblue", "tan",
          "teal", "thistle", "tomato", "turquoise", "violet", "wheat", "white",
          "whitesmoke"]


def print_plot(kind, data, count):
    fig = make_subplots(
        rows=2,
        cols=2,
        vertical_spacing=0.17,
        subplot_titles=[
            "Read bytes",
            "Write bytes",
            "Read IOPS",
            "Write IOPS"])

    clor_len = len(COLORS)
    index = 0

    percentile_set = set()
    for percentile_data in data:
        percentile_set.add(percentile_data["percentile"])
    sorted_percentile = sorted(percentile_set)

    for percentile in sorted_percentile:
        for percentile_data in data:
            if percentile_data["percentile"] != percentile:
                continue

            if percentile_data["type"] == 0:
                fig.add_trace(
                    graph_objects.Scatter(
                        x=[datetime.fromtimestamp(ts) for ts in percentile_data["data"]["x"]],
                        y=percentile_data["data"]["y"],
                        legendgroup=percentile_data["percentile"],
                        mode="lines",
                        line=dict(color=COLORS[index % clor_len]),
                        name="percentile_" + str(percentile_data["percentile"])),
                    row=1,
                    col=1)
            elif percentile_data["type"] == 2:
                fig.add_trace(
                    graph_objects.Scatter(
                        x=[datetime.fromtimestamp(ts) for ts in percentile_data["data"]["x"]],
                        y=percentile_data["data"]["y"],
                        legendgroup=percentile_data["percentile"],
                        mode="lines",
                        showlegend=False,
                        line=dict(color=COLORS[index % clor_len]),
                        name="percentile_" + str(percentile_data["percentile"])),
                    row=1,
                    col=2)
            elif percentile_data["type"] == 1:
                fig.add_trace(
                    graph_objects.Scatter(
                        x=[datetime.fromtimestamp(ts) for ts in percentile_data["data"]["x"]],
                        y=percentile_data["data"]["y"],
                        legendgroup=percentile_data["percentile"],
                        mode="lines",
                        showlegend=False,
                        line=dict(color=COLORS[index % clor_len]),
                        name="percentile_" + str(percentile_data["percentile"])),
                    row=2,
                    col=1)
            elif percentile_data["type"] == 3:
                fig.add_trace(
                    graph_objects.Scatter(
                        x=[datetime.fromtimestamp(ts) for ts in percentile_data["data"]["x"]],
                        y=percentile_data["data"]["y"],
                        legendgroup=percentile_data["percentile"],
                        mode="lines",
                        showlegend=False,
                        line=dict(color=COLORS[index % clor_len]),
                        name="percentile_" + str(percentile_data["percentile"])),
                    row=2,
                    col=2)
        index += 1

    fig.update_layout(
        title_text="<b>" + kind + "</b>",
        title_x=0.5,
        width=1800,
        height=900,
        title_font_size=30)

    html = fig.to_html(full_html=False)

    html += "<h2>Groups count for  " + kind + ":</h2>"
    html += str(count)

    html += "<h2>Top groups for " + kind + ":</h2>"
    for percentile in sorted_percentile:
        html += "<b>percentile_" + str(percentile) + ":</b>"
        for group in percentile_data["data"]["groups"]:
            html += "  " + str(group)
        html += "</br>"

    return html

from io import StringIO
import matplotlib
import numpy as np
import pandas as pd
from clan_tools.utils.time import utcms2datetime
matplotlib.use('Agg')
from matplotlib import pyplot as plt


def to_string_io(f):
    def wrapper(*args, **kwargs):
        plt.figure()
        io_img = StringIO()
        f(*args, **kwargs)
        plt.savefig(io_img, format='png')
        io_img.seek(0)
        return io_img

    return wrapper


@to_string_io
def plot_event(sensor_values_df, t_start, t_end):
    ax = sensor_values_df.plot()
    ax.set_xticklabels(sensor_values_df.index.map(
        lambda x: '{} {}:{}:{}'.format(x.day, x.hour, x.minute, x.second)))
    ax.axvline(x=utcms2datetime(t_start), color='r', linestyle='--', lw=2)
    ax.axvline(x=utcms2datetime(t_end), color='r', linestyle='--', lw=2)
    ax.set_ylabel('Bar')


def autolabel(rects, values, ax):
    for rect, value in zip(rects, values):
        label = '%d' % int(value) if value > 0 else ''
        ax.text(rect.get_x() + rect.get_width() / 2., value * 1.05,
                label,
                ha='center', va='bottom')


@to_string_io
def plot_stats(df):
    fig, ax = plt.subplots()
    severities = df['severity'].drop_duplicates().values
    index = df.index.drop_duplicates().values
    margin_bottom = np.zeros_like(index)
    index_df = pd.DataFrame(margin_bottom, index=index)
    width = 0.8
    rects = None
    for severity in severities:
        res_df = index_df.join(df[df['severity'] == severity]).fillna(0)
        freq = res_df.loc[:, 'events_started']
        x = np.arange(len(index))
        rects = ax.bar(x, freq, width, bottom=margin_bottom, label=severity)
        ax.set_ylabel('Number of events')
        margin_bottom += freq
        ax.set_ylim(0, np.max(margin_bottom) * 1.2)
        ax.set_xticks(np.add(x, 0))
        ax.set_xticklabels(index, rotation=90, ha="center")
    plt.legend()
    autolabel(rects, margin_bottom, ax)

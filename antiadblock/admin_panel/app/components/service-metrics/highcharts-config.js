import i18n from 'app/lib/i18n';

export const defaultConfig = {
    chart: {
        type: 'spline',
        zoomType: 'x'
    },
    yAxis: {
        title: {
            text: null
        },
        min: 0
    },
    xAxis: {
        type: 'datetime'
    },
    legend: {
        layout: 'vertical',
        align: 'right',
        verticalAlign: 'middle',
        width: 127
    },
    tooltip: {
        shared: true,
        crosshairs: true
    },
    plotOptions: {
        spline: {
            lineWidth: 2,
            marker: {
                enabled: false
            }
        }
    },
    credits: {
        enabled: false
    }
};

export const status = {
    ...defaultConfig,
    title: {
        text: i18n('graphs', 'rps')
    }
};

export const percents = {
    ...defaultConfig,
    chart: {
        ...defaultConfig.chart,
        type: 'column'
    },
    title: {
        text: i18n('graphs', 'http-errors')
    },
    yAxis: {
        title: {
            text: ''
        },
        labels: {
            formatter: function () {
                return this.value + '%';
            }
        },
        softMax: 6
    },
    plotOptions: {
        series: {
            stacking: 'normal'
        }
    }
};

export const domains = {
    ...defaultConfig,
    chart: {
        ...defaultConfig.chart,
        type: 'column'
    },
    title: {
        text: i18n('graphs', 'errors-by-partner')
    },
    yAxis: {
        title: {
            text: ''
        },
        softMax: 6
    },
    plotOptions: {
        series: {
            stacking: 'normal'
        }
    }
};

export const proportions = {
    ...defaultConfig,
    chart: {
        ...defaultConfig.chart,
        type: 'column'
    },
    title: {
        text: i18n('graphs', 'adblock-apps-proportions')
    },
    yAxis: {
        title: {
            text: ''
        },
        labels: {
            formatter: function () {
                return this.value + '%';
            }
        }
    },
    tooltip: {
        pointFormat: '<span style="color:{series.color}">{series.name}</span>: <b>{point.percentage:.0f}%</b><br/>',
        shared: true
    },
    plotOptions: {
        column: {
            stacking: 'percent'
        }
    }
};

export const blockers = {
    ...defaultConfig,
    chart: {
        ...defaultConfig.chart,
        type: 'line'
    },
    plotOptions: {
        line: {
            marker: {
                enabled: false
            }
        }
    },
    title: {
        text: i18n('graphs', 'adblockers')
    },
    yAxis: {
        title: {
            text: ''
        },
        labels: {
            formatter: function () {
                return this.value + '%';
            }
        },
        min: 0,
        softMax: 100
    }
};

export const browsers = {
    ...defaultConfig,
    chart: {
        ...defaultConfig.chart,
        type: 'line'
    },
    title: {
        text: i18n('graphs', 'browsers')
    },
    plotOptions: {
        line: {
            marker: {
                enabled: false
            }
        }
    },
    yAxis: {
        title: {
            text: ''
        },
        labels: {
            formatter: function () {
                return this.value + '%';
            }
        },
        min: 0,
        softMax: 100
    }
};

export const timings = {
    ...defaultConfig,
    title: {
        text: i18n('graphs', 'timings')
    },
    yAxis: {
        allowDecimals: false,
        minorTickInterval: 100,
        title: {
            text: ''
        },
        labels: {
            formatter: function () {
                if (this.value >= 1000) {
                    return (this.value / 1000) + 's';
                }
                return this.value + 'ms';
            }
        }
    },
    tooltip: {
        shared: true,
        crosshairs: true,
        pointFormatter: function () {
            let value = this.y + 'ms';

            if (this.y >= 1000) {
                value = (this.y / 1000) + 's';
            }

            return `<span style="color:${this.color}">\u25CF</span> ${this.series.name}: <b>${value}</b><br/>`;
        }
    },
    legend: {
        ...defaultConfig.legend,
        labelFormat: `{name} ${i18n('graphs', 'percentile')}`
    }
};

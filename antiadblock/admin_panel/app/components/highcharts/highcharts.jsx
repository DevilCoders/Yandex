import React from 'react';
import PropTypes from 'prop-types';

import ReactHighcharts from 'react-highcharts';

import i18n from 'app/lib/i18n';

// global configuration
ReactHighcharts.Highcharts.setOptions({
    global: {
        useUTC: false
    },
    lang: {
        resetZoom: i18n('graphs', 'reset-zoom'),
        resetZoomTitle: i18n('graphs', 'reset-zoom-title'),
        weekdays: [
            i18n('datetime', 'sunday'),
            i18n('datetime', 'monday'),
            i18n('datetime', 'tuesday'),
            i18n('datetime', 'wednesday'),
            i18n('datetime', 'thursday'),
            i18n('datetime', 'friday'),
            i18n('datetime', 'saturday')
        ],
        shortMonths: [
            i18n('datetime', 'jan'),
            i18n('datetime', 'feb'),
            i18n('datetime', 'mar'),
            i18n('datetime', 'apr'),
            i18n('datetime', 'may'),
            i18n('datetime', 'jun'),
            i18n('datetime', 'jul'),
            i18n('datetime', 'aug'),
            i18n('datetime', 'sept'),
            i18n('datetime', 'oct'),
            i18n('datetime', 'nov'),
            i18n('datetime', 'dec')
        ]
    }
});

export default class Highcharts extends React.Component {
    shouldComponentUpdate(nextProps) {
        return nextProps.loaded !== this.props.loaded ||
            nextProps.loading !== this.props.loading;
    }

    render() {
        const {config, loading, loaded} = this.props;

        const series = config && config.plotOptions && config.plotOptions.series;

        // Вычисляем диапозон для оси X. Округляем значение до минут
        const now = new Date();
        const milliseconds = now.getTime() - now.getMilliseconds();
        const minMax = {
            max: milliseconds,
            min: milliseconds - (this.props.range * 60 * 1000)
        };

        let focus = null;

        return (
            <ReactHighcharts
                config={{
                    ...config,
                    xAxis: {
                        ...config.xAxis,
                        ...minMax
                    },
                    chart: {
                        ...config.chart,
                        events: {
                            load: function () {
                                // Если взведен флаг loading, то показываем сообщение о загрузке данных
                                if (loading) {
                                    this.showLoading(i18n('graphs', 'loading'));
                                    return;
                                }

                                // Если loading - false и loaded - false, то показываем ошибку загрузки данных
                                if (!loaded) {
                                    this.showLoading(i18n('graphs', 'server-error'));
                                    return;
                                }

                                // Если loading - false, а loaded - true, то данные пришли
                                // Если они пустые, то пишем сообщение no data
                                if (!this.series || !this.series.length) {
                                    this.showLoading(i18n('graphs', 'no-data'));
                                }
                            }
                        }
                    },
                    plotOptions: {
                        ...config.plotOptions,
                        series: {
                            ...series,
                            events: {
                                // Повторяем поведение из графаны. При щелчке в легенде скрываем все линии, кроме текущей
                                legendItemClick: function (e) {
                                    const chart = this.chart;
                                    if (focus !== this) {
                                        // Скрываем все линии, кроме текущей
                                        for (let i = 0; i < chart.series.length; i++) {
                                            chart.series[i].setVisible(chart.series[i] === this, false);
                                        }

                                        focus = this;
                                    } else {
                                        // Показываем все линии
                                        for (let i = 0; i < chart.series.length; i++) {
                                            chart.series[i].setVisible(true, false);
                                        }

                                        focus = null;
                                    }
                                    chart.redraw();
                                    e.preventDefault();
                                }
                            }
                        }
                    },
                    series: this.props.series
                }} />
        );
    }
}

Highcharts.propTypes = {
    range: PropTypes.number,
    config: PropTypes.object,
    series: PropTypes.array,
    loading: PropTypes.bool,
    loaded: PropTypes.bool
};

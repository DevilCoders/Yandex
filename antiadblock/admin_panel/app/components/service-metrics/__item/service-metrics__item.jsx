import React from 'react';
import {connect} from 'react-redux';
import PropTypes from 'prop-types';

import {fetchMetric} from 'app/actions/metric';
import {getMetric} from 'app/reducers/index';

import {GRAPHS, RANGE, COLORS} from 'app/enums/graphs';

import Bem from 'app/components/bem/bem';
import Select from 'lego-on-react/src/components/select/select.react';
import ReactHighcharts from 'app/components/highcharts/highcharts';

import i18n from 'app/lib/i18n';

import * as config from '../highcharts-config';

import './service-metrics__item.css';

const TOTAL_TITLE = 'total';

class ServiceMetricItem extends React.Component {
    constructor(props) {
        super(props);

        this._scope = {};

        this.state = {
            seriesMap: {},
            // Выбираем начальное значение range
            range: RANGE[props.ranges[0]]
        };

        this.setRange = this.setRange.bind(this);
        this.setWrapperRef = this.setWrapperRef.bind(this);
    }

    setWrapperRef(wrapper) {
        this._scope.dom = wrapper;
    }

    componentWillReceiveProps(nextProps) {
        let newState = {};

        const nextMetrics = nextProps.metrics;
        const currentMetrics = this.props.metrics;

        this.props.graphs.forEach(graph => {
            const metricName = GRAPHS[graph];

            const nextLoaded = nextMetrics[metricName].loaded;
            const currentLoaded = currentMetrics[metricName].loaded;

            if (nextLoaded !== currentLoaded) {
                newState[graph] = this.proceed(nextMetrics[metricName], graph);
            }
        });

        this.setState(state => ({
            seriesMap: {
                ...state.seriesMap,
                ...newState
            }
        }));
    }

    getMetricsList() {
        // Исключаем одинаковые элементы из списка
        return [...new Set(this.props.graphs.map(item => GRAPHS[item]))];
    }

    componentDidMount() {
        // В начале запрашиваем все метрики
        this.props.fetchMetrics(this.props.service.id, this.getMetricsList(), this.state.range);
    }

    proceed(data, name) {
        if (!data || !data.data || !data.loaded) {
            return [];
        }

        // При range не больше 1 часа данные агрегируются по минутам. Для получения RPS нужно поделить на 60
        let divisor = 60;
        const range = this.state.range;

        switch (range) {
            // При range (1, 12] часов интервалы - 5м
            case RANGE['4h']:
            case RANGE['12h']:
                divisor *= 5;
                break;
            // При range (12, 24] часа интервалы - 30м
            case RANGE['24h']:
                divisor *= 30;
                break;
            // При range больше 24 часов интервалы - 60м
            case RANGE['4d']:
            case RANGE['7d']:
                divisor *= 60;
                break;
            default:
        }

        switch (name) {
            case 'percents':
                return this.collectPercents(data.data);
            case 'status':
                return this.handleServerData(data.data, divisor, true, true);
            default:
                return this.handleServerData(data.data);
        }
    }

    /**
     * Преобразуем данные от сервера для отображения отношений в виде columns в highcharts
     * @param {Object} data - данные с сервера
     * @returns {Array} массив серий
     */
    collectPercents(data) {
        const collected = this.handleServerData(data);

        // Собираем общую сумму на каждой точке
        const total = {};
        collected.forEach(series => {
            series.data.forEach(item => {
                if (!total[item[0]]) {
                    total[item[0]] = 0;
                }

                total[item[0]] += item[1];
            });
        });

        // На графике хотим видеть только ошибки -> HTTP коды >= 400
        return collected
            .filter(item => parseInt(item.name, 10) >= 400)
            .map(obj => {
                return {
                    ...obj,
                    data: obj.data.map(item => [item[0], parseFloat(((item[1] / total[item[0]]) * 100).toFixed(3))])
                };
            });
    }

    /**
     * Преобразуем данные от сервера в формат понятный highcharts
     * [{name: '*', data: [[x, y], [x, y], ..., [x, y]]}]
     * @param {Object} data - данные с сервера
     * @param {Number} modifier - все значения делятся на modifier. Это необходимо, тк RPS приходят агрегированными по минутам (часам)
     * @param {Boolean} removeLastPoint - удалить последнюю точку. На некоторых графиках в последней точке недостаточно информации и ее лушче удалить
     * @param {Boolean} collectTotal - нужно ли добавлять отдельную линию с суммой всех остальных
     * @returns {Array} массив серий
     */
    handleServerData(data, modifier, removeLastPoint, collectTotal) {
        const series = {};

        // данные могут придти за минуты или даже за часы, нужно привести в секунды
        modifier = modifier || 1;

        let total = {},
            result;

        Object.keys(data).forEach(time => {
            const statuses = data[time];
            Object.keys(statuses).forEach(status => {
                if (!series[status]) {
                    series[status] = {
                        name: status,
                        data: []
                    };
                }

                if (!total[time]) {
                    total[time] = [parseInt(time, 10), 0];
                }

                total[time][1] += statuses[status];

                const value = parseFloat((statuses[status] / modifier).toFixed(3));
                series[status].data.push([parseInt(time, 10), value]);
            });
        });

        result = Object.values(series);

        // Если необходимо, то добавляем отдельную линию total с суммой
        if (collectTotal && result.length) {
            result = [
                ...result,
                {
                    name: TOTAL_TITLE,
                    data: Object.values(total).map(value => {
                        return [value[0], parseFloat((value[1] / modifier).toFixed(3))];
                    })
                }
            ];
        }

        return result
            .map(item => {
                // Фиксируем цвета
                item.color = COLORS[item.name];

                if (removeLastPoint) {
                    item.data.pop();
                }

                return item;
            });
    }

    setRange(values) {
        // setRange присылает массив
        const value = values[0];
        if (this.state.range !== value) {
            this.setState({
                range: value
            });

            // Обновляем только графики у которых поменялся range
            this.props.fetchMetrics(this.props.service.id, this.getMetricsList(), value);
        }
    }

    render() {
        return (
            <Bem
                block='service-metric'
                elem='block'
                tagRef={this.setWrapperRef}>
                <Bem
                    block='service-metric'
                    elem='select'>
                    <Select
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='m'
                        width='max'
                        type='radio'
                        val={this.state.range}
                        popup={{
                            scope: this._scope.dom
                        }}
                        onChange={this.setRange}>
                        {this.props.ranges.map(range => {
                            return (
                                <Select.Item
                                    val={RANGE[range]}
                                    key={range} >
                                    {i18n('graphs', `last-${range}`)}
                                </Select.Item>
                            );
                        })}
                    </Select>
                </Bem>

                <Bem
                    block='service-metric'
                    elem='flex'>
                    {this.props.graphs.map(graph => {
                        const metricName = GRAPHS[graph];
                        return (
                            <Bem
                                key={graph}
                                block='service-metric'
                                elem='plot'
                                mix={{
                                    block: 'service-metric',
                                    elem: graph
                                }}>
                                <ReactHighcharts
                                    loaded={this.props.metrics[metricName].loaded}
                                    loading={this.props.metrics[metricName].loading}
                                    series={this.state.seriesMap[graph] || []}
                                    config={config[graph]}
                                    range={this.state.range} />
                            </Bem>
                        );
                    })}
                </Bem>
            </Bem>
        );
    }
}

ServiceMetricItem.propTypes = {
    service: PropTypes.object,
    metrics: PropTypes.object,
    graphs: PropTypes.array,
    ranges: PropTypes.array,
    fetchMetrics: PropTypes.func
};

export default connect((state, props) => {
    return {
        metrics: getMetric(state, props.service.id)
    };
}, dispatch => {
    return {
        fetchMetrics: (serviceId, metrics, range) => {
            return Promise.all(metrics.map(metric => {
                return dispatch(fetchMetric(serviceId, metric, range));
            }));
        }
    };
})(ServiceMetricItem);

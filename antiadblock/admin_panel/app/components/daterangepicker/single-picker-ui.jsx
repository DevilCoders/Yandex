import React from 'react';
import PropTypes from 'prop-types';

import {INTERVAL} from 'app/enums/daterangepicker';
import {dateLibDayJsType} from 'app/types';
import {dayjs, getMonthNamesForLocale} from 'app/lib/date';

import Bem from 'app/components/bem/bem';
import DaterangepickerSingleWrapper from './__single-wrapper/daterangepicker__single-wrapper';
import Button from 'lego-on-react/src/components/button/button.react';

import i18n from 'app/lib/i18n';

import './daterangepicker.css';

const dateStartSbs = [2019, 10, 5];

export default class SinglePickerUI extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            ...this.getDefaultState(props)
        };

        this.onDayMouseEnter = this.onDayMouseEnter.bind(this);
        this.onDayClick = this.onDayClick.bind(this);
        this.onApply = this.onApply.bind(this);
        this.autoApply = this.autoApply.bind(this);
        this.renderCalendar = this.renderCalendar.bind(this);
        this.onChangeStartDate = this.onChangeStartDate.bind(this);
        this.onEscapeDate = this.onEscapeDate.bind(this);
        this.forceApply = this.forceApply.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        if (this.props.initDate !== nextProps.initDate) {
            this.setState({
                startDate: nextProps.initDate
            });
        }
    }

    getDefaultState(props) {
        const calendar = dayjs();
        const startOfWeek = calendar.startOf('week');
        const endOfWeek = calendar.endOf('week');
        const startDate = props.initDate;
        const range = startDate;
        const minDate = dayjs(dateStartSbs);
        const maxDate = dayjs();
        const autoApply = false;
        const showDropdowns = true;
        const closedOrOpen = INTERVAL.CLOSED;
        const minYear = minDate.year();
        const maxYear = maxDate.year();
        const showWeekNumbers = true;
        const showISOWeekNumbers = false;
        const ranges = {};
        const weekLabel = 'Н';
        const linkedCalendars = true;

        const weekNames = [];
        for (let s = startOfWeek; s <= endOfWeek;) {
            weekNames.push(s.format('dd'));
            s = s.add(1, 'day');
        }

        const language = 'ru';
        const monthNames = getMonthNamesForLocale(language);
        const locale = {
            weekNames,
            language,
            monthNames,
            weekLabel
        };

        return {
            locale,
            calendar,
            startDate,
            minDate,
            maxDate,
            showDropdowns,
            minYear,
            maxYear,
            showWeekNumbers,
            showISOWeekNumbers,
            ranges,
            linkedCalendars,
            autoApply,
            weekLabel,
            closedOrOpen,
            range
        };
    }

    onDayMouseEnter(day) {
        const {
            startDate
        } = this.state;
        const range = (day >= startDate) ? day : startDate;
        this.setState({range});
    }

    onDayClick(day) {
        let startDate,
            options = {};

        startDate = day;
        options.closedOrOpen = INTERVAL.CLOSED;

        const range = startDate;

        this.setState({
            startDate,
            range,
            ...options
        });

        setTimeout(this.autoApply, 0);
    }

    onEscapeDate() {
        this.setState({
            startDate: null,
            closedOrOpen: INTERVAL.CLOSED,
            range: {}
        });

        setTimeout(this.forceApply, 0);
    }

    autoApply() {
        if (this.props.autoApply) {
            this.onApply();
        }
    }

    forceApply() {
        const {
            startDate
        } = this.state;
        const {
            onDatesChange
        } = this.props;

        if (onDatesChange) {
            onDatesChange({startDate});
        }
    }

    onChangeStartDate(day) {
        this.setState({
            startDate: day,
            closedOrOpen: INTERVAL.CLOSED,
            range: day
        });

        setTimeout(this.autoApply, 0);
    }

    onApply() {
        const {
            startDate
        } = this.state;
        const {
            onDatesChange
        } = this.props;

        if (onDatesChange && startDate) {
            onDatesChange({startDate});
        }
    }

    dateLabel() {
        const {
            startDate
        } = this.state;

        return startDate ? `${startDate.format('DD.MM.YYYY')}` : '';
    }

    renderCalendar() {
        const {
            onDayClick,
            onDayMouseEnter
        } = this;
        const props = {
            hideRange: true,
            onDayClick,
            onDayMouseEnter,
            ...this.state,
            ...this.props,
            onDatesChange: () => {},
            endDate: this.state.startDate
        };
        const {
            component: Component
        } = this.props;
        const {autoApply} = props;

        return (
            <Component {...props}>
                {!autoApply &&
                    <Bem block='daterangepicker-actions'>
                        <Bem
                            block='daterangepicker-actions'
                            elem='interval'>
                            {this.dateLabel()}
                        </Bem>
                        <Button
                            view='default'
                            tone='grey'
                            size='xs'
                            theme='normal'
                            mix={{
                                block: 'daterangepicker-actions',
                                elem: 'button'
                            }}>
                            {i18n('common', 'cancel')}
                        </Button>
                        <Button
                            onClick={this.onApply}
                            view='default'
                            tone='grey'
                            type='action'
                            size='xs'
                            theme='action'
                            mix={{
                                block: 'daterangepicker-actions',
                                elem: 'button'
                            }}>
                            {i18n('common', 'apply')}
                        </Button>
                    </Bem>
                }
            </Component>
        );
    }

    render() {
        const {
            onDayClick,
            onDayMouseEnter
        } = this;
        const props = {
            onDayClick,
            onDayMouseEnter,
            ...this.state,
            ...this.props,
            onDatesChange: () => {},
            endDate: this.state.startDate
        };

        return (
            <DaterangepickerSingleWrapper
                {...props}
                onEscapeDate={this.onEscapeDate}
                onChangeStartDate={this.onChangeStartDate}>
                {this.renderCalendar()}
            </DaterangepickerSingleWrapper>
        );
    }
}

SinglePickerUI.propTypes = {
    initDate: dateLibDayJsType,
    onDatesChange: PropTypes.func,
    component: PropTypes.node,
    autoApply: PropTypes.bool,
    onlyFrom: PropTypes.bool
};

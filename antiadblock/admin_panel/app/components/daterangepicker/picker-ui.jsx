import React from 'react';
import PropTypes from 'prop-types';

import {INTERVAL} from 'app/enums/daterangepicker';
import {dayjs, getMonthNamesForLocale} from 'app/lib/date';

import Bem from 'app/components/bem/bem';
import DaterangepickerWrapper from './__wrapper/daterangepicker__wrapper';
import Button from 'lego-on-react/src/components/button/button.react';

import i18n from 'app/lib/i18n';

import './daterangepicker.css';

const dateStartSbs = [2019, 10, 5];

export default class PickerUI extends React.Component {
    constructor() {
        super();

        this.state = {
            ...this.getDefaultState()
        };

        this.onDayMouseEnter = this.onDayMouseEnter.bind(this);
        this.onDayClick = this.onDayClick.bind(this);
        this.onApply = this.onApply.bind(this);
        this.autoApply = this.autoApply.bind(this);
        this.renderCalendar = this.renderCalendar.bind(this);
        this.onChangeEndDate = this.onChangeEndDate.bind(this);
        this.onChangeStartDate = this.onChangeStartDate.bind(this);
        this.onEscapeDate = this.onEscapeDate.bind(this);
        this.forceApply = this.forceApply.bind(this);
    }

    getDefaultState() {
        const calendar = dayjs();
        const startOfWeek = calendar.startOf('week');
        const endOfWeek = calendar.endOf('week');
        const startDate = null;
        const endDate = null;
        const range = (endDate || startDate) || {};
        const minDate = dayjs(dateStartSbs);
        const maxDate = dayjs();
        const autoApply = false;
        const showDropdowns = true;
        const closedOrOpen = false;
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
            endDate,
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
            startDate,
            endDate
        } = this.state;
        const range = (day >= startDate) ? endDate || day : startDate;
        this.setState({range});
    }

    onDayClick(day) {
        let startDate,
            endDate,
            options = {};

        const isOpen = this.state.closedOrOpen === INTERVAL.OPEN;
        startDate = isOpen ? this.state.startDate : day;
        endDate = !isOpen ? null : day;
        options.closedOrOpen = isOpen ? INTERVAL.CLOSED : INTERVAL.OPEN;

        if (startDate !== null && endDate !== null && startDate > endDate) {
            [startDate, endDate] = [endDate, startDate];
        }

        const range = endDate || startDate;

        this.setState({
            startDate,
            endDate,
            range,
            ...options
        });

        setTimeout(this.autoApply, 0);
    }

    onEscapeDate() {
        this.setState({
            startDate: null,
            endDate: null,
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
            startDate,
            endDate
        } = this.state;
        const {
            onDatesChange
        } = this.props;

        if (onDatesChange) {
            onDatesChange({startDate, endDate});
        }
    }

    onChangeStartDate(day) {
        let {
            endDate
        } = this.state,
            option = {};

        if (endDate && endDate < day) {
            [endDate, day] = [day, endDate];
            option.endDate = endDate;
        }

        this.setState({
            startDate: day,
            closedOrOpen: endDate ? INTERVAL.CLOSED : INTERVAL.OPEN,
            range: day,
            ...option
        });

        setTimeout(this.autoApply, 0);
    }

    onChangeEndDate(day) {
        let {
            startDate
        } = this.state,
            option = {};

        if (startDate && day < startDate) {
            [startDate, day] = [day, startDate];
            option.startDate = startDate;
        }

        this.setState({
            endDate: day,
            closedOrOpen: startDate ? INTERVAL.CLOSED : INTERVAL.OPEN,
            range: day
        });

        setTimeout(this.autoApply, 0);
    }

    onApply() {
        const {
            startDate,
            endDate
        } = this.state;
        const {
            onDatesChange
        } = this.props;

        if (onDatesChange && startDate && endDate) {
            onDatesChange({startDate, endDate});
        }
    }

    dateLabel() {
        const {
            startDate,
            endDate
        } = this.state;

        return startDate && endDate ? `${startDate.format('DD.MM.YYYY')} - ${endDate.format('DD.MM.YYYY')}` : '';
    }

    renderCalendar() {
        const {
            onDayClick,
            onDayMouseEnter
        } = this;
        const props = {
            onDayClick,
            onDayMouseEnter,
            ...this.state,
            ...this.props,
            onDatesChange: () => {}
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
            onDatesChange: () => {}
        };

        return (
            <DaterangepickerWrapper
                {...props}
                onEscapeDate={this.onEscapeDate}
                onChangeEndDate={this.onChangeEndDate}
                onChangeStartDate={this.onChangeStartDate}>
                {this.renderCalendar()}
            </DaterangepickerWrapper>
        );
    }
}

PickerUI.propTypes = {
    onDatesChange: PropTypes.func,
    component: PropTypes.node,
    autoApply: PropTypes.bool,
    onlyFrom: PropTypes.bool
};

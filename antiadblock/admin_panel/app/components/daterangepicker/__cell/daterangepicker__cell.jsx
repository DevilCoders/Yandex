import React from 'react';
import PropTypes from 'prop-types';

import {Td} from 'app/components/table/table-components';

import {UNIT_TYPE} from 'app/enums/daterangepicker';
import {dateLibDayJsType} from 'app/types';
import * as dates from 'app/lib/daterangepicker';

export class DaterangepickerCell extends React.Component {
    constructor() {
        super();

        this.handleOnMouseEnter = this.handleOnMouseEnter.bind(this);
        this.handleOnClick = this.handleOnClick.bind(this);
    }

    handleOnMouseEnter() {
        const {
            onDayMouseEnter,
            day,
            minDate,
            maxDate
        } = this.props;

        if (onDayMouseEnter) {
            const prepareDay = dates.getValueOnInterVal(minDate, maxDate, day);

            onDayMouseEnter(prepareDay);
        }
    }

    handleOnClick() {
        const {
            onDayClick,
            day,
            minDate,
            maxDate
        } = this.props;

        if (onDayClick) {
            const prepareDay = dates.getValueOnInterVal(minDate, maxDate, day);

            onDayClick(prepareDay);
        }
    }

    getModsCell() {
        const {
            day,
            calendar,
            startDate,
            endDate,
            range,
            maxDate,
            minDate,
            hideRange
        } = this.props;

        const weekend = dates.isWeekend(day);
        const inRange = (
            dates.isBetweenExclusive(startDate, endDate, day) ||
            dates.isBetweenExclusive(startDate, range, day)
        );
        const isEndDate = endDate && dates.dateDayJsIsEqual(day, endDate);
        const isStartDate = startDate && dates.dateDayJsIsEqual(day, startDate);
        const active = isStartDate || isEndDate;
        const off = (
            calendar.month() !== day.month() ||
            dates.isNotBetween(minDate, maxDate, day)
        );
        const disabled = dates.isNotBetween(minDate, maxDate, day);

        return {
            weekend,
            'in-range': hideRange ? null : inRange,
            'end-date': isEndDate,
            'start-date': isStartDate,
            active,
            off,
            disabled,
            available: !disabled
        };
    }

    render() {
        const {
            day
        } = this.props;
        const mods = this.getModsCell();

        return (
            <Td
                block='daterangepicker-table'
                elem='cell'
                mods={{
                    body: true,
                    ...mods
                }}
                onClick={this.handleOnClick}
                onMouseEnter={this.handleOnMouseEnter}>
                {day.format('DD')}
            </Td>
        );
    }
}

export class WeekCell extends React.Component {
    render() {
        const {
            week
        } = this.props;

        return (
            <Td
                block='daterangepicker-table'
                elem='cell'
                mods={{
                    body: true,
                    week: true
                }}>
                {week}
            </Td>
        );
    }
}

export const cellMapper = {
    [UNIT_TYPE.DAY]: DaterangepickerCell,
    [UNIT_TYPE.WEEK]: WeekCell
};

DaterangepickerCell.propTypes = {
    day: dateLibDayJsType.isRequired,
    startDate: dateLibDayJsType,
    endDate: dateLibDayJsType,
    minDate: dateLibDayJsType,
    maxDate: dateLibDayJsType,
    calendar: dateLibDayJsType.isRequired,
    range: dateLibDayJsType,
    onDayClick: PropTypes.func,
    onDayMouseEnter: PropTypes.func,
    hideRange: PropTypes.bool
};

WeekCell.propTypes = {
    week: PropTypes.number
};

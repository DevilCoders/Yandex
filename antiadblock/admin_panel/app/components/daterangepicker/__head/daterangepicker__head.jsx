import React from 'react';
import PropTypes from 'prop-types';

import {THead, Tr, Th} from 'app/components/table/table-components';
import DaterangepickerNext from '../__next/daterangepicker__next';
import DaterangepickerPrev from '../__prev/daterangepicker__prev';
import DaterangepickerTitle from '../__title/daterangepicker__title';

import {dateLibDayJsType} from 'app/types';

export default class DaterangepickerHeader extends React.Component {
    createNextProps() {
        const {
            showNext,
            calendar,
            nextDate,
            handleNext
        } = this.props;
        const date = nextDate(calendar);

        return {
            next: showNext,
            calendar: date,
            handleNext
        };
    }

    createPrevProps() {
        const {
            showPrev,
            calendar,
            prevDate,
            handlePrev
        } = this.props;
        const date = prevDate(calendar);

        return {
            prev: showPrev,
            calendar: date,
            handlePrev
        };
    }

    renderTitle() {
        const {
            showISOWeekNumbers,
            showWeekNumbers,
            calendar,
            locale,
            showDropdowns,
            maxYear,
            minYear,
            handleSelected
        } = this.props;
        const colSpan = showISOWeekNumbers || showWeekNumbers ? 6 : 5;
        const props = {
            colSpan,
            calendar,
            locale,
            maxYear,
            minYear,
            showDropdowns,
            handleSelected
        };

        return <DaterangepickerTitle {...props} />;
    }

    renderNext() {
        const props = this.createNextProps();
        return <DaterangepickerNext {...props} />;
    }

    renderPrev() {
        const props = this.createPrevProps();
        return <DaterangepickerPrev {...props} />;
    }

    renderWeeks() {
        const {
            showWeekNumbers,
            showISOWeekNumbers,
            locale
        } = this.props;
        const {
            weekNames,
            weekLabel
        } = locale;

        const showWeeks = showISOWeekNumbers || showWeekNumbers;
        const weeks = showWeeks ? [weekLabel].concat(weekNames) : weekNames;
        const WeekData = weeks.map((children, key) => {
            return (
                <Th
                    block='daterangepicker-table'
                    elem='cell'
                    key={`header-cell-${key === 0 && showWeeks ? 'week' : 'day'}-${children}`}
                    mods={{
                        header: true,
                        week: key === 0 && showWeeks
                    }}>
                    {children}
                </Th>
            );
        });
        return (
            <Tr
                block='daterangepicker-table'
                elem='row'
                mods={{
                    header: true
                }}>
                {WeekData}
            </Tr>
        );
    }

    render() {
        return (
            <THead
                block='daterangepicker-table'
                elem='header'>
                <Tr
                    block='daterangepicker-table'
                    elem='row'
                    mods={{
                        header: true,
                        action: true
                    }}>
                    {this.renderPrev()}
                    {this.renderTitle()}
                    {this.renderNext()}
                </Tr>
                {this.renderWeeks()}
            </THead>
        );
    }
}

DaterangepickerHeader.propTypes = {
    calendar: dateLibDayJsType.isRequired,
    locale: PropTypes.object.isRequired,
    handleNext: PropTypes.func.isRequired,
    handlePrev: PropTypes.func.isRequired,
    showISOWeekNumbers: PropTypes.bool,
    showWeekNumbers: PropTypes.bool,
    nextDate: PropTypes.func,
    prevDate: PropTypes.func,
    showNext: PropTypes.bool,
    showPrev: PropTypes.bool,
    handleSelected: PropTypes.func,
    showDropdowns: PropTypes.bool,
    maxYear: PropTypes.number,
    minYear: PropTypes.number
};

DaterangepickerHeader.defaultProps = {
    showISOWeekNumbers: false,
    showWeekNumbers: false,
    showDropdowns: false,
    showNext: true,
    showPrev: true,
    nextDate: calendar => calendar.add(1, 'month'),
    prevDate: calendar => calendar.subtract(1, 'month')
};

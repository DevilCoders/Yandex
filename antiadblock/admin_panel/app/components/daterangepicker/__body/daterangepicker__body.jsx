import React from 'react';
import PropTypes from 'prop-types';

import {Tr, TBody} from 'app/components/table/table-components';
import {cellMapper} from '../__cell/daterangepicker__cell';

import * as dates from 'app/lib/daterangepicker';
import {UNIT_TYPE} from 'app/enums/daterangepicker';
import {dateLibDayJsType} from 'app/types';

export default class DaterangepickerBody extends React.Component {
    getResolvedData() {
        const {
            startDate,
            endDate,
            range,
            minDate,
            maxDate,
            hideRange,
            showWeekNumbers,
            showISOWeekNumbers,
            calendar,
            onDayClick,
            onDayMouseEnter
        } = this.props;

        const startOfMonth = calendar.startOf('month');
        const startOfWeek = startOfMonth.startOf('week');
        let s = startOfWeek.clone();

        // календарь представляет из себя матрицу 6x7
        // заполняется с даты начала начала недели до 42 дня
        const resolvedData = [];
        for (let i = 0; i < 6; i++) {
            const rows = [];

            if (showISOWeekNumbers || showWeekNumbers) {
                const w = s.clone().set('day', i * 7);
                const week = showISOWeekNumbers ?
                    dates.isoWeek(w.toDate()) :
                    w.week();
                rows.push({
                    unitType: UNIT_TYPE.WEEK,
                    key: 0,
                    week
                });
            }

            for (let j = 0; j < 7; j++) {
                const props = {
                    unitType: UNIT_TYPE.DAY,
                    minDate,
                    maxDate,
                    hideRange,
                    day: s,
                    calendar,
                    startDate,
                    range,
                    endDate,
                    onDayMouseEnter,
                    onDayClick
                };
                s = s.add(1, 'day');
                rows.push(props);
            }

            resolvedData.push({
                rows,
                rowsKey: `body-rows-days-${i}`
            });
        }

        return resolvedData;
    }

    renderCells() {
        const resolvedData = this.getResolvedData();

        return resolvedData.map(data => {
            return (
                <Tr
                    key={data.rowsKey}
                    block='daterangepicker-table'
                    elem='row'
                    mods={{
                        body: true
                    }}>
                    {data.rows.map(props => {
                        const Component = cellMapper[props.unitType];
                        return (
                            <Component
                                key={`body-cell-day-${props.day}`}
                                {...props} />
                        );
                    })}
                </Tr>
            );
        });
    }

    render() {
        return (
            <TBody
                block='daterangepicker-table'
                elem='body'>
                {this.renderCells()}
            </TBody>
        );
    }
}

DaterangepickerBody.propTypes = {
    calendar: dateLibDayJsType.isRequired,
    startDate: dateLibDayJsType,
    endDate: dateLibDayJsType,
    minDate: dateLibDayJsType,
    maxDate: dateLibDayJsType,
    hideRange: PropTypes.bool,
    showISOWeekNumbers: PropTypes.bool.isRequired,
    showWeekNumbers: PropTypes.bool.isRequired,
    onDayClick: PropTypes.func,
    onDayMouseEnter: PropTypes.func,
    range: PropTypes.object
};

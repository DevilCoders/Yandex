import React from 'react';
import PropTypes from 'prop-types';

import {Th} from 'app/components/table/table-components';
import Select from 'lego-on-react/src/components/select/select.react';

import {dayjs} from 'app/lib/date';
import {dateLibDayJsType} from 'app/types';

const TYPE_DATA = {
    month: 1,
    year: 2
};

export default class DaterangepickerTitle extends React.Component {
    constructor(props) {
        super(props);

        this.onChangeDate = this.onChangeDate.bind(this);
    }

    onChangeDate(dateType) {
        const {
            calendar,
            handleSelected
        } = this.props;

        return handleSelected ? value => {
            const option = dateType === TYPE_DATA.year ? {
                selectedYear: value[0],
                selectedMonth: calendar.month()
            } : dateType === TYPE_DATA.month ? {
                selectedYear: calendar.year(),
                selectedMonth: value[0]
            } : {
                selectedYear: calendar.year(),
                selectedMonth: calendar.month()
            };
            const newCalendar = dayjs([option.selectedYear, option.selectedMonth + 1]);

            handleSelected(newCalendar);
        } : () => {};
    }

    getYearNames() {
        const {
            maxYear,
            minYear
        } = this.props;

        let sY = parseInt(minYear, 10);
        const eY = parseInt(maxYear, 10);
        const yearNames = [];

        for (; sY < eY + 1; sY++) {
            yearNames.push(sY);
        }

        return yearNames;
    }

    render() {
        const {
            calendar,
            colSpan,
            locale,
            showDropdowns
        } = this.props;
        const month = calendar.month();
        const year = calendar.year();
        let label = calendar.format('MMM YYYY');

        if (showDropdowns) {
            const {monthNames} = locale;
            const yearNames = this.getYearNames();

            label = [
                <Select
                    key='select-month'
                    theme='normal'
                    view='classic'
                    size='xs'
                    type='radio'
                    val={month}
                    onChange={this.onChangeDate(TYPE_DATA.month)}>
                    {monthNames.map((name, key) => (
                        <Select.Item
                            val={key}
                            key={`item-${name}`} >
                            {name}
                        </Select.Item>
                    ))}
                </Select>,
                <Select
                    key='select-year'
                    theme='normal'
                    view='classic'
                    size='xs'
                    type='radio'
                    val={year}
                    onChange={this.onChangeDate(TYPE_DATA.year)}>
                    {yearNames.map(name => (
                        <Select.Item
                            val={name}
                            key={`item-${name}`} >
                            {name}
                        </Select.Item>
                    ))}
                </Select>
            ];
        }

        return (
            <Th
                colSpan={colSpan}
                block='daterangepicker-table'
                elem='cell'
                mods={{
                    body: true,
                    week: true
                }}>
                {label}
            </Th>
        );
    }
}

DaterangepickerTitle.propTypes = {
    calendar: dateLibDayJsType.isRequired,
    colSpan: PropTypes.number.isRequired,
    handleSelected: PropTypes.func.isRequired,
    showDropdowns: PropTypes.bool.isRequired,
    locale: PropTypes.object,
    minYear: PropTypes.number,
    maxYear: PropTypes.number
};

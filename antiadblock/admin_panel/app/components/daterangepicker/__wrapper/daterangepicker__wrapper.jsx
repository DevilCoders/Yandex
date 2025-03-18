import React from 'react';
import PropTypes from 'prop-types';

import isEqual from 'lodash/isEqual';
import {dayjs} from 'app/lib/date';
import i18n from 'app/lib/i18n';
import {dateLibDayJsType} from 'app/types';
import {dateDayJsIsEqual} from 'app/lib/daterangepicker';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Popup from 'lego-on-react/src/components/popup/popup.react';
import TextInput from 'lego-on-react/src/components/textinput/textinput.react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import './daterangepicker__wrapper.css';

const INPUT_TYPES = {
    dayFrom: 'dayFrom',
    monthFrom: 'monthFrom',
    yearFrom: 'yearFrom',
    dayTo: 'dayTo',
    monthTo: 'monthTo',
    yearTo: 'yearTo'
};

export default class DaterangepickerWrapper extends React.Component {
    constructor(props) {
        super(props);

        const {
            startDate,
            endDate
        } = props;

        const newFromTo = this.prepareDateFromCalendarToInputs({startDate, endDate});

        this.state = {
            calendarIsVisible: false,
            validation: '',
            ...newFromTo
        };

        this._anchor = null;

        this.setAnchorRef = this.setAnchorRef.bind(this);

        this.onChangeCalendarVisible = this.onChangeCalendarVisible.bind(this);
        this.onChangeDateFromInput = this.onChangeDateFromInput.bind(this);
        this.onBlurInputsChangeCalendar = this.onBlurInputsChangeCalendar.bind(this);
        this.onClearAll = this.onClearAll.bind(this);
        this.onCloseCalendar = this.onCloseCalendar.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        const {
            startDate,
            endDate
        } = nextProps;

        if (!isEqual(this.props.startDate, nextProps.startDate) || !isEqual(this.props.endDate, nextProps.endDate)) {
            this.setState(state => ({
                ...this.prepareDateFromCalendarToInputs({startDate, endDate}),
                calendarIsVisible: !(nextProps.startDate && nextProps.endDate) && state.calendarIsVisible,
                validation: startDate && endDate ? '' : state.validation
            }));
        }
    }

    prepareDateFromCalendarToInputs(date) {
        const {
            startDate,
            endDate
        } = date;

        const from = startDate ? {
            dayFrom: this.prepareDayOrMonth(startDate.date()),
            monthFrom: this.prepareDayOrMonth(startDate.month() + 1),
            yearFrom: String(startDate.year())
        } : {
            ...this.getDefaultStateDateFrom()
        };

        const to = endDate ? {
            dayTo: this.prepareDayOrMonth(endDate.date()),
            monthTo: this.prepareDayOrMonth(endDate.month() + 1),
            yearTo: String(endDate.year())
        } : {
            ...this.getDefaultStateDateTo()
        };

        return {
            ...to,
            ...from
        };
    }

    prepareDateFromInputsToCalendar(date) {
        const {
            minDate,
            maxDate,
            minYear,
            maxYear
        } = this.props;

        const {
            day,
            month,
            year
        } = date;

        let newYear = year,
            newMonth = month,
            newDay = day,
            newDate;

        if (year > maxYear) {
            newYear = maxYear;
        } else if (year < minYear) {
            newYear = minYear;
        }

        if (month > 12) {
            newMonth = 12;
        } else if (month < 1) {
            newMonth = 1;
        }

        const daysInMonth = dayjs([newYear, newMonth]).daysInMonth();

        if (day > daysInMonth) {
            newDay = daysInMonth;
        } else if (day < 1) {
            newDay = 1;
        }

        newDate = dayjs([newYear, newMonth, newDay]);

        if (newDate < minDate) {
            newDate = minDate;
        } else if (newDate > maxDate) {
            newDate = maxDate;
        }

        return newDate;
    }

    setAnchorRef(ref) {
        this._anchor = ref;
    }

    getDefaultStateDateFrom() {
        return {
            dayFrom: '',
            monthFrom: '',
            yearFrom: ''
        };
    }

    getDefaultStateDateTo() {
        return {
            dayTo: '',
            monthTo: '',
            yearTo: ''
        };
    }

    prepareDayOrMonth(number) {
        return number < 10 ? '0' + number : String(number);
    }

    dateToNumber(date) {
        return {
            day: Number(date.day),
            month: Number(date.month),
            year: Number(date.year)
        };
    }

    dateIsValid(date) {
        return Boolean(date.day.match(/^\d+$/) && date.month.match(/^\d+$/) && date.year.match(/^\d+$/));
    }

    dateIsEmpty(date) {
       return Boolean(date.day === '' && date.month === '' && date.year === '');
    }

    onBlurInputsChangeCalendar() {
        const {
            dayFrom,
            monthFrom,
            yearFrom,
            dayTo,
            monthTo,
            yearTo,
            validation
        } = this.state;

        const from = {day: dayFrom, month: monthFrom, year: yearFrom};
        const to = {day: dayTo, month: monthTo, year: yearTo};

        if ((!this.dateIsValid(from) && !this.dateIsEmpty(from)) || (!this.dateIsValid(to) && !this.dateIsEmpty(to))) {
            this.setState({
                validation: i18n('sbs', 'daterangepicker-validation')
            });
        } else if (validation) {
            this.setState({
                validation: ''
            });
        }

        if (this.dateIsValid(from)) {
            const startDate = this.prepareDateFromInputsToCalendar(this.dateToNumber(from));

            // если данные не изменились, не перерисовывать все
            if (!dateDayJsIsEqual(this.props.startDate, startDate)) {
                this.props.onChangeStartDate(startDate);
            }
        }

        if (this.dateIsValid(to)) {
            const endDate = this.prepareDateFromInputsToCalendar(this.dateToNumber(to));

            // если данные не изменились, не перерисовывать все
            if (!dateDayJsIsEqual(this.props.endDate, endDate)) {
                this.props.onChangeEndDate(endDate);
            }
        }
    }

    onChangeDateFromInput(input) {
        return value => {
            this.setState({
                [input]: value
            });
        };
    }

    onClearAll() {
        this.setState({
            calendarIsVisible: false,
            validation: '',
            // сбрасываем значения в инпутах
            ...this.getDefaultStateDateFrom(),
            ...this.getDefaultStateDateTo()
        });

        this.props.onEscapeDate();
    }

    onCloseCalendar() {
        this.setState({
           calendarIsVisible: false
        });

        const {
            dayFrom,
            monthFrom,
            yearFrom,
            dayTo,
            monthTo,
            yearTo
        } = this.state;
        const {
            maxDate,
            minDate,
            onDayClick
        } = this.props;

        // если одна из границ не введена, то заполняем ее крайним значением
        if (dayFrom && monthFrom && yearFrom && !(dayTo && monthTo && yearTo)) {
            onDayClick(maxDate);
        } else if (dayTo && monthTo && yearTo && !(dayFrom && monthFrom && yearFrom)) {
            onDayClick(minDate);
        }
    }

    onChangeCalendarVisible(value) {
        return e => {
            if (e.target.select) {
                e.target.select();
            }

            this.setState({
                calendarIsVisible: value
            });
        };
    }

    render() {
        const {
            dayFrom,
            monthFrom,
            yearFrom,
            dayTo,
            monthTo,
            yearTo,
            calendarIsVisible,
            validation
        } = this.state;

        return (
            <Bem
                block='daterangepicker-wrapper'
                ref={this.setAnchorRef}>
                <Bem
                    block='daterangepicker-wrapper'
                    elem='form'
                    mods={
                        validation ? {
                            error: true
                        } : {}
                    }>
                    <TextInput
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'input-day'
                        }}
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='s'
                        placeholder='--'
                        onChange={this.onChangeDateFromInput(INPUT_TYPES.dayFrom)}
                        onFocus={this.onChangeCalendarVisible(true)}
                        onBlur={this.onBlurInputsChangeCalendar}
                        text={dayFrom}
                        pin='round-clear' />
                    <Bem
                        block='daterangepicker-wrapper'
                        elem='split'>
                        /
                    </Bem>
                    <TextInput
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'input-month'
                        }}
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='s'
                        placeholder='--'
                        onChange={this.onChangeDateFromInput(INPUT_TYPES.monthFrom)}
                        onFocus={this.onChangeCalendarVisible(true)}
                        onBlur={this.onBlurInputsChangeCalendar}
                        text={monthFrom}
                        pin='clear-clear' />
                    <Bem
                        block='daterangepicker-wrapper'
                        elem='split'>
                        /
                    </Bem>
                    <TextInput
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'input-year'
                        }}
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='s'
                        placeholder='----'
                        onChange={this.onChangeDateFromInput(INPUT_TYPES.yearFrom)}
                        onFocus={this.onChangeCalendarVisible(true)}
                        onBlur={this.onBlurInputsChangeCalendar}
                        text={yearFrom}
                        pin='clear-clear' />
                    <Bem
                        block='daterangepicker-wrapper'
                        elem='split'>
                        –
                    </Bem>
                    <TextInput
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'input-day'
                        }}
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='s'
                        placeholder='--'
                        onChange={this.onChangeDateFromInput(INPUT_TYPES.dayTo)}
                        onFocus={this.onChangeCalendarVisible(true)}
                        onBlur={this.onBlurInputsChangeCalendar}
                        text={dayTo}
                        pin='clear-clear' />
                    <Bem
                        block='daterangepicker-wrapper'
                        elem='split'>
                        /
                    </Bem>
                    <TextInput
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'input-month'
                        }}
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='s'
                        placeholder='--'
                        onChange={this.onChangeDateFromInput(INPUT_TYPES.monthTo)}
                        onFocus={this.onChangeCalendarVisible(true)}
                        onBlur={this.onBlurInputsChangeCalendar}
                        text={monthTo}
                        pin='clear-clear' />
                    <Bem
                        block='daterangepicker-wrapper'
                        elem='split'>
                        /
                    </Bem>
                    <TextInput
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'input-year'
                        }}
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='s'
                        placeholder='----'
                        onChange={this.onChangeDateFromInput(INPUT_TYPES.yearTo)}
                        onFocus={this.onChangeCalendarVisible(true)}
                        onBlur={this.onBlurInputsChangeCalendar}
                        text={yearTo}
                        pin='clear-clear' />
                    <Button
                        theme='normal'
                        view='default'
                        size='s'
                        pin='clear-clear'
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'clear-button'
                        }}
                        onClick={this.onClearAll}>
                        <Icon
                            glyph='type-cross'
                            size='xs' />
                    </Button>
                    <Button
                        theme='normal'
                        view='default'
                        size='s'
                        pin='clear-clear'
                        mix={{
                            block: 'daterangepicker-wrapper',
                            elem: 'show-button'
                        }}
                        onClick={this.onChangeCalendarVisible(!calendarIsVisible)}>
                        <Icon
                            glyph='type-filter'
                            size='xs' />
                    </Button>
                </Bem>
                <Popup
                    anchor={this._anchor}
                    autoclosable
                    onOutsideClick={this.onCloseCalendar}
                    onClose={this.onCloseCalendar}
                    visible={this.state.calendarIsVisible}
                    theme='normal'>
                    {this.props.children}
                </Popup>
                <Tooltip
                    anchor={this._anchor}
                    scope={this.context.scope && this.context.scope.dom}
                    view='classic'
                    tone='default'
                    theme='error'
                    to={['right']}
                    mainOffset={10}
                    mix={{
                        block: 'config-text-editor',
                        elem: 'tooltip'
                    }}
                    visible={Boolean(validation)}>
                    {validation}
                </Tooltip>
            </Bem>
        );
    }
}

DaterangepickerWrapper.propTypes = {
    startDate: dateLibDayJsType,
    endDate: dateLibDayJsType,
    minDate: dateLibDayJsType,
    maxDate: dateLibDayJsType,
    onChangeEndDate: PropTypes.func,
    onChangeStartDate: PropTypes.func,
    onDayClick: PropTypes.func,
    onEscapeDate: PropTypes.func,
    maxYear: PropTypes.number,
    minYear: PropTypes.number,
    children: PropTypes.node
};

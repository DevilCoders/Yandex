import React from 'react';
import PropTypes from 'prop-types';

import {dayjs} from 'app/lib/date';

import {Table} from 'app/components/table/table-components';
import DaterangepickerBody from './__body/daterangepicker__body';
import DaterangepickerHeader from './__head/daterangepicker__head';
import PickerUI from './picker-ui';
import Bem from '../bem/bem';

class LinkedCalendar extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            leftCalendar: dayjs().subtract(1, 'month'),
            rightCalendar: dayjs()
        };

        this.handlePrev = this.handlePrev.bind(this);
        this.handleNext = this.handleNext.bind(this);
    }

    createProps() {
        const {
            maxYear,
            minYear
        } = this.props;
        const {
            leftCalendar,
            rightCalendar
        } = this.state;
        const {
            handlePrev,
            handleNext
        } = this;

        // функции (handleNext, handleNext) - пустые, во избежании ошибок
        // так как по этим функциям сдвигается календарь и происходит перерисовка
        return {
            leftProps: {
                handlePrev,
                handleNext: () => {
                },
                handleSelected: handlePrev,
                showNext: false,
                showPrev: minYear <= leftCalendar.subtract(1, 'month').year(),
                ...this.props,
                calendar: leftCalendar
            },
            rightProps: {
                showPrev: false,
                showNext: rightCalendar.add(1, 'month').year() <= maxYear,
                handlePrev: () => {
                },
                handleNext,
                handleSelected: handleNext,
                ...this.props,
                calendar: rightCalendar
            }
        };
    }

    handlePrev(leftCalendar) {
        this.setState({
            leftCalendar,
            rightCalendar: leftCalendar.add(1, 'month')
        });
    }

    handleNext(rightCalendar) {
        this.setState({
            rightCalendar,
            leftCalendar: rightCalendar.subtract(1, 'month')
        });
    }

    renderTable() {
        const props = this.createProps();
        const {
            leftProps,
            rightProps
        } = props;

        return [
            <Bem
                block='daterangepicker-calendar'
                key={0}>
                <Table
                    block='daterangepicker-calendar'
                    elem='table'
                    mix={{
                        block: 'daterangepicker-table'
                    }}>
                    <DaterangepickerHeader {...leftProps} />
                    <DaterangepickerBody {...leftProps} />
                </Table>
            </Bem>,
            <Bem
                block='daterangepicker-calendar'
                key={1}>
                <Table
                    block='daterangepicker-calendar'
                    elem='table'
                    mix={{
                        block: 'daterangepicker-table'
                    }}>
                    <DaterangepickerHeader {...rightProps} />
                    <DaterangepickerBody {...rightProps} />
                </Table>
            </Bem>
        ];
    }

    render() {
        const {
            children
        } = this.props;

        return (
            <Bem
                block='daterangepicker'>
                {this.renderTable()}
                {children}
            </Bem>
        );
    }
}

LinkedCalendar.propTypes = {
    children: PropTypes.node.isRequired,
    maxYear: PropTypes.number,
    minYear: PropTypes.number
};

export default class LinkedCalendarUI extends React.Component {
    render() {
        const uiProps = {
            ...this.props,
            component: LinkedCalendar
        };

        return (
            <PickerUI {...uiProps} />
        );
    }
}


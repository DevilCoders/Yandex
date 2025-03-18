import React from 'react';
import PropTypes from 'prop-types';

import {dayjs} from 'app/lib/date';

import Bem from 'app/components/bem/bem';
import DaterangepickerBody from './__body/daterangepicker__body';
import DaterangepickerHeader from './__head/daterangepicker__head';
import PickerUI from './picker-ui';
import {Table} from 'app/components/table/table-components';

class UnlinkedCalendar extends React.Component {
    constructor() {
        super();

        this.state = {
            leftCalendar: dayjs().subtract(1, 'month'),
            rightCalendar: dayjs()
        };
    }

    createProps() {
        const {
            leftCalendar,
            rightCalendar
        } = this.state;
        const {
            handlePrev,
            handleNext
        } = this;

        return {
            leftProps: {
                handlePrev,
                handleNext: handlePrev,
                handleSelected: handlePrev,
                ...this.props,
                calendar: leftCalendar
            },
            rightProps: {
                handleNext,
                handleSelected: handleNext,
                handlePrev: handleNext,
                ...this.props,
                calendar: rightCalendar
            }
        };
    }

    handlePrev(leftCalendar) {
        this.setState({
            leftCalendar
        });
    }

    handleNext(rightCalendar) {
        this.setState({
            rightCalendar
        });
    }

    renderTable() {
        const props = this.createProps();
        const {leftProps, rightProps} = props;

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

export default class UnlinkedCalendarUI extends React.Component {
    render() {
        const uiProps = {
            ...this.props,
            component: UnlinkedCalendar
        };

        return (
            <PickerUI {...uiProps} />
        );
    }
}

UnlinkedCalendar.propTypes = {
    children: PropTypes.node.isRequired
};


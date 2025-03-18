import React from 'react';
import PropTypes from 'prop-types';

import {dayjs} from 'app/lib/date';

import Bem from 'app/components/bem/bem';
import {Table} from 'app/components/table/table-components';
import DaterangepickerBody from './__body/daterangepicker__body';
import DaterangepickerHeader from './__head/daterangepicker__head';
import PickerUI from './picker-ui';

class Calendar extends React.Component {
    constructor() {
        super();

        this.state = {
            calendar: dayjs()
        };

        this.handleSelected = this.handleSelected.bind(this);
        this.handleNext = this.handleNext.bind(this);
        this.handlePrev = this.handlePrev.bind(this);
    }

    handleSelected(calendar) {
        this.setState({
            calendar
        });
    }

    createProps() {
        const {
            handlePrev,
            handleNext,
            handleSelected
        } = this;

        return {
            handlePrev,
            handleNext,
            handleSelected,
            ...this.props,
            ...this.state
        };
    }

    handleNext(calendar) {
        this.setState({
            calendar
        });
    }

    handlePrev(calendar) {
        this.setState({
            calendar
        });
    }

    renderTable() {
        const props = this.createProps();

        return (
            <Bem
                block='daterangepicker-calendar'>
                <Table
                    block='daterangepicker-calendar'
                    elem='table'
                    mix={{
                        block: 'daterangepicker-table'
                    }}>
                    <DaterangepickerHeader {...props} />
                    <DaterangepickerBody {...props} />
                </Table>
            </Bem>
        );
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

export default class CalendarUI extends React.Component {
    render() {
        const uiProps = {
            ...this.props,
            component: Calendar
        };

        return (
            <PickerUI {...uiProps} />
        );
    }
}

Calendar.propTypes = {
    children: PropTypes.node.isRequired
};

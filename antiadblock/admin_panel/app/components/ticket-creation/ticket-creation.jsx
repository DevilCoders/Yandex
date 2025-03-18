import React from 'react';
import PropTypes from 'prop-types';
import connect from 'react-redux/es/connect/connect';

import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Modal from 'lego-on-react/src/components/modal/modal.react';
import {SingleCalendar} from 'app/components/daterangepicker';
import ModalClose from 'app/components/modal/__close/modal__close';

import {closeTicketCreation} from 'app/actions/ticket-creation';
import {setGlobalErrors} from 'app/actions/errors';
import {createTrend} from 'app/actions/service';

import {dayjs} from 'app/lib/date';

import {getTicketCreation} from 'app/reducers/index';

import './ticket-creation.css';

class TicketCreation extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            date: dayjs(),
            progress: false,
            errors: ''
        };

        this.onApply = this.onApply.bind(this);
        this.onChangeDates = this.onChangeDates.bind(this);
        this.onApply = this.onApply.bind(this);
        this.onModalClose = this.onModalClose.bind(this);
        this.setErrors = this.setErrors.bind(this);
    }

    onApply() {
        if (!this.state.date) {
            this.setErrors(i18n('service-page', 'date-empty'));
            return;
        }

        this.setState({
            progress: true
        });

        const date = this.state.date ? this.state.date.toDate() : null;

        this.props.createTrend(this.props.serviceId, date)
            .then(() => this.onModalClose(),
                result => {
                    this.setState({
                        progress: false
                    });

                    if (result.status === 400) {
                        const response = result.response;
                        this.setErrors(response.message);
                    } else {
                        this.props.setGlobalErrors([result.message]);
                    }
                });
    }

    onModalClose() {
        this.setState({
            progress: false,
            date: dayjs(),
            errors: ''
        });
        this.props.closeTicketCreation();
    }

    onChangeDates(value) {
        this.setState({
            date: value.startDate,
            errors: ''
        });
    }

    setErrors(error) {
        this.setState({
            errors: error
        });
    }

    render() {
        return (
            <Modal
                autoclosable
                onDocKeyDown={this.onModalClose}
                onOutsideClick={this.onModalClose}
                visible={this.props.visible}>
                <Bem
                    block='modal'
                    elem='title'>
                    {i18n('service-page', 'create-ticket')}
                </Bem>
                <Bem
                    block='ticket-creation'
                    mods={{
                        'has-errors': Boolean(this.state.errors)
                    }}>
                    <Bem
                        block='ticket-creation'
                        elem='calendar'>
                        <Bem
                            block='ticket-creation'
                            elem='title'>
                            {i18n('service-page', 'start-date')}:
                        </Bem>
                        <Bem
                            block='ticket-creation'
                            elem='row'>
                            <SingleCalendar
                                onlyFrom
                                autoApply
                                showDropdowns
                                initDate={this.state.date}
                                onDatesChange={this.onChangeDates} />
                            <Bem
                                block='ticket-creation'
                                elem='error'>
                                {this.state.errors}
                            </Bem>
                        </Bem>
                    </Bem>
                </Bem>
                <Bem
                    key='actions'
                    block='modal'
                    elem='actions'>
                    <Button
                        theme='action'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        progress={this.state.progress}
                        onClick={this.onApply}>
                        {i18n('common', 'apply')}
                    </Button>
                    <Button
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onModalClose}>
                        {i18n('common', 'cancel')}
                    </Button>
                </Bem>
                <ModalClose
                    onClick={this.onModalClose} />
            </Modal>
        );
    }
}

TicketCreation.propTypes = {
    closeTicketCreation: PropTypes.func,
    createTrend: PropTypes.func,
    setGlobalErrors: PropTypes.func,
    visible: PropTypes.bool,
    serviceId: PropTypes.any
};

export default connect(state => {
    const ticketConfig = getTicketCreation(state);
    return {
        visible: ticketConfig.visible,
        serviceId: ticketConfig.serviceId
    };
}, dispatch => {
    return {
        createTrend: (serviceId, datetime) => {
            return dispatch(createTrend(serviceId, datetime));
        },
        closeTicketCreation: () => {
            dispatch(closeTicketCreation());
        },
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(TicketCreation);

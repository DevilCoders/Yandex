import React from 'react';
import PropTypes from 'prop-types';
import connect from 'react-redux/es/connect/connect';

import {setGlobalErrors} from 'app/actions/errors';

import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';
import ModalClose from 'app/components/modal/__close/modal__close';

import Button from 'lego-on-react/src/components/button/button.react';
import Modal from 'lego-on-react/src/components/modal/modal.react';
import TextInput from 'lego-on-react/src/components/textinput/textinput.react';

import {fetchLabels} from 'app/actions/service';
import {closeLabelCreateModal} from 'app/actions/create-label';

import {getCreateLabel} from 'app/reducers/index';

import servicesApi from 'app/api/service';

import {KEYS} from 'app/enums/keys';

import './label-create.css';

class LabelCreate extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            comment: '',
            errors: ''
        };

        this.onKeyUp = this.onKeyUp.bind(this);
        this.onApply = this.onApply.bind(this);
        this.onModalClose = this.onModalClose.bind(this);
        this.onChange = this.onChange.bind(this);
        this.setIconRef = this.setIconRef.bind(this);
    }

    onKeyUp(e) {
        if (e.keyCode === KEYS.ENTER) {
            this.onApply();
        }
    }

    onApply() {
        servicesApi.createLabel(this.state.comment.trim(), this.props.labelId)
            .then(() => {
                this.props.fetchLabels(this.props.serviceId);
                this.onModalClose();
            }, result => {
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
            comment: '',
            errors: ''
        });
        this.props.closeLabelCreateModal();
    }

    onChange(value) {
        this.setState({
            comment: value
        });
    }

    setIconRef(icon) {
        this._icon = icon;
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
                    {i18n('service-page', 'label-create')}
                </Bem>
                <Bem
                    block='label-create'
                    mods={{
                        'has-errors': Boolean(this.state.errors)
                    }}>
                    <Bem
                        block='label-create'
                        elem='comment'
                        ref={this.setIconRef}>
                        <TextInput
                            controlAttrs={{
                                onKeyUp: this.onKeyUp
                            }}
                            theme='normal'
                            tone='grey'
                            view='default'
                            size='m'
                            text={this.state.comment}
                            placeholder={i18n('placeholders', 'label-create')}
                            onChange={this.onChange} />
                        <Bem
                            block='label-create'
                            elem='error' >
                            {this.state.errors}
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
                        onClick={this.onApply}>
                        {i18n('common', 'create')}
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

LabelCreate.propTypes = {
    labelId: PropTypes.string,
    visible: PropTypes.bool,
    serviceId: PropTypes.string,
    fetchLabels: PropTypes.func.isRequired,
    closeLabelCreateModal: PropTypes.func.isRequired,
    setGlobalErrors: PropTypes.func.isRequired
};

export default connect(state => {
    return {
        visible: getCreateLabel(state).visible,
        serviceId: getCreateLabel(state).serviceId,
        labelId: getCreateLabel(state).labelId
    };
}, dispatch => {
    return {
        fetchLabels: serviceId => {
            dispatch(fetchLabels(serviceId));
        },
        closeLabelCreateModal: id => {
            dispatch(closeLabelCreateModal(id));
        },
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(LabelCreate);

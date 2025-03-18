import React from 'react';
import PropTypes from 'prop-types';

import {setGlobalErrors} from 'app/actions/errors';

import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';

import ModalClose from 'app/components/modal/__close/modal__close';
import Button from 'lego-on-react/src/components/button/button.react';
import Modal from 'lego-on-react/src/components/modal/modal.react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import TextArea from 'lego-on-react/src/components/textarea/textarea.react';

import connect from 'react-redux/es/connect/connect';
import {getConfigDeclining} from 'app/reducers';
import {closeConfigDeclining} from 'app/actions/config-declining';
import {endModerateServiceConfig} from 'app/actions/service';

import configApi from 'app/api/config';

import './config-declining.css';

class ConfigDeclining extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            comment: '',
            errors: ''
        };

        this.onApply = this.onApply.bind(this);
        this.onModalClose = this.onModalClose.bind(this);
        this.onChange = this.onChange.bind(this);
        this.setIconRef = this.setIconRef.bind(this);
    }

    onApply() {
        configApi.setModerateConfig(this.props.labelId, this.props.id, false, this.state.comment)
            .then(config => {
                this.props.endModerateServiceConfig(this.props.serviceId, this.props.id, false, this.state.comment, config);
                this.onModalClose();
            }, result => {
                if (result.status === 400) {
                    const response = result.response;
                    this.setErrors(response.properties[0].message);
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
        this.props.closeConfigDeclining(this.props.id);
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
                    {i18n('service-page', 'declining-title')}
                </Bem>
                <Bem
                    block='config-declining'
                    mods={{
                        'has-errors': Boolean(this.state.errors)
                    }}>
                    <Bem
                        block='config-declining'
                        elem='comment'
                        ref={this.setIconRef}>
                        <TextArea
                            theme='normal'
                            tone='grey'
                            view='default'
                            size='m'
                            text={this.state.comment}
                            placeholder={i18n('service-page', 'config-form-comment-title')}
                            onChange={this.onChange}
                            onEnter={this.onApply} />
                    </Bem>
                    <Tooltip
                        anchor={this._icon}
                        view='classic'
                        tone='default'
                        theme='error'
                        to={['right-center']}
                        mainOffset={10}
                        zIndexGroupLevel={100}
                        mix={{
                            block: 'modal',
                            elem: 'tooltip'
                        }}
                        visible={Boolean(this.state.errors)}>
                        {this.state.errors}
                    </Tooltip>
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

ConfigDeclining.propTypes = {
    id: PropTypes.number,
    labelId: PropTypes.string,
    visible: PropTypes.bool,
    serviceId: PropTypes.string,
    endModerateServiceConfig: PropTypes.func.isRequired,
    closeConfigDeclining: PropTypes.func.isRequired,
    setGlobalErrors: PropTypes.func
};

export default connect(state => {
    const config = getConfigDeclining(state);

    return {
        id: config.id,
        labelId: config.labelId,
        visible: config.visible,
        serviceId: config.serviceId,
        comment: config.comment
    };
}, dispatch => {
    return {
        endModerateServiceConfig: (serviceId, configId, approved, comment, config) => {
            dispatch(endModerateServiceConfig(serviceId, configId, approved, comment, config));
        },
        closeConfigDeclining: id => {
            dispatch(closeConfigDeclining(id));
        },
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(ConfigDeclining);

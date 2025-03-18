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

import {closeExperimentApplying} from 'app/actions/experiment-applying';
import {applyExperiment} from 'app/actions/service';
import {getExperimentApplying} from 'app/reducers/index';

import './experiment-applying.css';

// TODO в другом PR создали enum KEYS, нужно не забыть исправить
const KEYS = {
    ENTER: 13
};

class ExperimentApplying extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            comment: '',
            errors: '',
            progress: false
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
        this.setState({
            progress: true
        });

        this.props.applyExperiment(this.props.serviceId, this.props.configId, {
            comment: this.state.comment.trim()
        })
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
            comment: '',
            errors: ''
        });
        this.props.closeExperimentApplying();
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
                    {i18n('service-page', 'experiment-applying')}
                </Bem>
                <Bem
                    block='experiment-applying'
                    mods={{
                        'has-errors': Boolean(this.state.errors)
                    }}>
                    <Bem
                        block='experiment-applying'
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
                            placeholder={i18n('placeholders', 'experiment-applying')}
                            onChange={this.onChange} />
                        <Bem
                            block='experiment-applying'
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

ExperimentApplying.propTypes = {
    visible: PropTypes.bool.isRequired,
    configId: PropTypes.number,
    serviceId: PropTypes.string,
    closeExperimentApplying: PropTypes.func.isRequired,
    setGlobalErrors: PropTypes.func.isRequired,
    applyExperiment: PropTypes.func.isRequired
};

export default connect(state => {
    const configExperimentAppling = getExperimentApplying(state);
    return {
        visible: configExperimentAppling.visible,
        configId: configExperimentAppling.configId,
        serviceId: configExperimentAppling.serviceId
    };
}, dispatch => {
    return {
        applyExperiment: (serviceId, configId, options) => {
            return dispatch(applyExperiment(serviceId, configId, options));
        },
        closeExperimentApplying: id => {
            dispatch(closeExperimentApplying(id));
        },
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(ExperimentApplying);

import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import YAML from 'js-yaml';
import {MonacoDiffEditor} from 'react-monaco-editor';

import Bem from 'app/components/bem/bem';

import Modal from 'lego-on-react/src/components/modal/modal.react';
import Button from 'lego-on-react/src/components/button/button.react';

import ModalClose from 'app/components/modal/__close/modal__close';
import LinkCopy from 'app/components/link-copy/link-copy';

import Preloader from '../preloader/preloader';
import {configType} from '../../types/index';

import {fetchCurrentConfig, closeConfigApplying} from 'app/actions/config-applying';
import {applyConfig} from 'app/actions/service';
import {getConfigApplying} from 'app/reducers/index';

import {KEYS} from 'app/enums/keys';

import i18n from 'app/lib/i18n';

import './config-applying.css';
import 'app/components/icon/_theme/icon_theme_link.css';

class ConfigApplying extends React.Component {
    constructor() {
        super();

        this.onApply = this.onApply.bind(this);
        this.onModalClose = this.onModalClose.bind(this);
        this.onKeyUp = this.onKeyUp.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        if (!this.props.visible && nextProps.visible) {
            this.props.fetchCurrentConfig(nextProps.configData.label_id, nextProps.target);
            document.addEventListener('keyup', this.onKeyUp);
        } else if (this.props.visible && !nextProps.visible) {
            document.removeEventListener('keyup', this.onKeyUp);
        }
    }

    onApply() {
        this.props.applyConfig(this.props.configData.label_id, this.props.id, {
            target: this.props.target,
            oldConfigId: this.props.oldConfigData.id
        });
        this.props.closeConfigApplying(this.props.id);
    }

    onModalClose() {
        this.props.closeConfigApplying(this.props.id);
    }

    getLinkDiffConfigToCopyUrl() {
        const {serviceId, configData, oldConfigData} = this.props;

        return `https://${location.host}/service/${serviceId}/configs/diff/${oldConfigData.id}/${configData.id}`;
    }

    onKeyUp(e) {
        if (e.keyCode === KEYS.ENTER) {
            this.onApply();
        }
    }

    render() {
        return (
            <Modal
                autoclosable
                onDocKeyDown={this.onModalClose}
                onOutsideClick={this.onModalClose}
                visible={this.props.visible}>
                {!this.props.loaded ?
                    <Preloader /> :
                    [
                        <Bem
                            key='title'
                            block='modal'
                            elem='title'>
                            {i18n('service-page', 'applying-title')}
                            &nbsp;
                            <LinkCopy
                                text={i18n('hints', 'link-copied-to-buffer')}
                                to='right'
                                value={this.getLinkDiffConfigToCopyUrl()} />
                        </Bem>,
                        <ModalClose
                            key='close'
                            onClick={this.onModalClose} />,
                        <Bem
                            key='body'
                            block='modal'
                            elem='body'>
                            <MonacoDiffEditor
                                language='yaml'
                                options={{
                                    readOnly: true,
                                    scrollBeyondLastLine: false,
                                    minimap: {
                                        enabled: false
                                    }
                                }}
                                original={YAML.safeDump(this.props.oldConfigData.data, {
                                    lineWidth: Infinity
                                })}
                                value={YAML.safeDump(this.props.configData.data, {
                                    lineWidth: Infinity
                                })} />
                        </Bem>,
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
                    ]}
            </Modal>
        );
    }
}

ConfigApplying.propTypes = {
    id: PropTypes.number,
    visible: PropTypes.bool,
    serviceId: PropTypes.string,
    target: PropTypes.string,
    configData: configType,
    oldConfigData: configType,
    loaded: PropTypes.bool,
    fetchCurrentConfig: PropTypes.func.isRequired,
    applyConfig: PropTypes.func.isRequired,
    closeConfigApplying: PropTypes.func.isRequired
};

export default connect(state => {
    return {
        id: getConfigApplying(state).id,
        visible: getConfigApplying(state).visible,
        serviceId: getConfigApplying(state).serviceId,
        target: getConfigApplying(state).target,
        configData: getConfigApplying(state).configData,
        oldConfigData: getConfigApplying(state).oldConfigData,
        loaded: getConfigApplying(state).loaded
    };
}, dispatch => {
    return {
        fetchCurrentConfig: (labelId, target) => {
            dispatch(fetchCurrentConfig(labelId, target));
        },
        applyConfig: (labelId, configId, target) => {
            dispatch(applyConfig(labelId, configId, target));
        },
        closeConfigApplying: id => {
            dispatch(closeConfigApplying(id));
        }
    };
})(ConfigApplying);

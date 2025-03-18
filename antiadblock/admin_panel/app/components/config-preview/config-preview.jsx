import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import {Link} from 'react-router-dom';

import YAML from 'js-yaml';
import ConfigEditor from 'app/components/config-editor/config-editor';
import ConfigYamlEditor from '../config-yaml-editor/config-yaml-editor';
import Bem from 'app/components/bem/bem';
import Modal from 'lego-on-react/src/components/modal/modal.react';
import Button from 'lego-on-react/src/components/button/button.react';
import RadioButton from 'lego-on-react/src/components/radio-button/radio-button.react';
import ModalClose from 'app/components/modal/__close/modal__close';

import {closeConfigPreview} from 'app/actions/config-preview';
import {fetchSchema} from 'app/actions/schema';
import {getConfigPreview} from 'app/reducers/index';
import {getService} from 'app/reducers/index';
import {getSchema} from 'app/reducers/service';

import {configType} from 'app/types';
import {ACTIONS} from 'app/enums/actions';

import {getUser} from 'app/lib/user';
import {getPermissions} from 'app/lib/permissions';
import {antiadbUrl} from 'app/lib/url';
import i18n from 'app/lib/i18n';
import {mergeConfig, getSchemaTypeAndDefVal, clearDefaultValues} from 'app/lib/config-editor';

import './config-preview.css';

const EDITOR = {
    FORM: 'form',
    YAML: 'yaml'
};

class ConfigPreview extends React.Component {
    constructor() {
        super();

        this.state = {
            editor: EDITOR.FORM
        };

        this.onModalClose = this.onModalClose.bind(this);
        this.setEditor = this.setEditor.bind(this);
    }

    componentWillUpdate(nextProps) {
        if (nextProps.configData && this.props.configData && nextProps.configData.label_id !== this.props.configData.label_id && nextProps.configData) {
            this.props.fetchSchema(nextProps.configData.label_id);
        }
    }

    onModalClose() {
        this.props.closeConfigPreview(this.props.id);
    }

    setEditor(e) {
        this.setState({
            editor: e.target.value
        });
    }

    render() {
        const user = getUser();
        const permissions = getPermissions();
        let dataSchema = {},
            data = {},
            dataCurrent = {};

        if (this.props.configData && this.props.schema.loaded) {
            dataSchema = getSchemaTypeAndDefVal(this.props.schema.schema || []);
            dataCurrent = clearDefaultValues(this.props.configData.data, this.props.configData.data_settings, dataSchema.values);
            data = mergeConfig(dataSchema.values, dataCurrent);
        }

        return (
            <Modal
                autoclosable
                onDocKeyDown={this.onModalClose}
                onOutsideClick={this.onModalClose}
                visible={this.props.visible}>
                {this.props.configData && this.props.schema.loaded ?
                    [
                        <Bem
                            key='title'
                            block='modal'
                            elem='title'>
                            {i18n('service-page', 'preview-title')} &#34;{this.props.configData.comment}&#34;
                        </Bem>,
                        <ModalClose
                            key='close'
                            onClick={this.onModalClose} />,
                        <Bem
                            key='radio'
                            block='config-preview'
                            elem='radio'>
                            <RadioButton
                                name='tone'
                                theme='normal'
                                size='s'
                                view='default'
                                tone='default'
                                value={this.state.editor}
                                onChange={this.setEditor} >
                                <RadioButton.Radio value={EDITOR.FORM}>
                                    {i18n('service-page', 'form-editor')}
                                </RadioButton.Radio>
                                <RadioButton.Radio value={EDITOR.YAML}>
                                    {i18n('service-page', 'yaml-editor')}
                                </RadioButton.Radio>
                            </RadioButton>
                        </Bem>,
                        <Bem
                            key='body'
                            block='modal'
                            elem='body'>
                            {this.state.editor === EDITOR.FORM ?
                                <ConfigEditor
                                    data={data}
                                    dataSettings={this.props.configData.data_settings}
                                    dataCurrent={dataCurrent}
                                    schema={this.props.schema.schema}
                                    onError={this.onErrorData}
                                    readOnly /> :
                                <ConfigYamlEditor
                                    data={YAML.safeDump(dataCurrent, {
                                        lineWidth: Infinity
                                    })}
                                    dataSettings={YAML.safeDump(this.props.configData.data_settings, {
                                        lineWidth: Infinity
                                    })}
                                    readOnly />}
                        </Bem>,
                        <Bem
                            key='actions'
                            block='modal'
                            elem='actions'>
                            {user.hasPermission(permissions.CONFIG_CREATE) && (
                                <Link
                                    to={antiadbUrl(`/service/${this.props.serviceId}/label/${this.props.configData.label_id}/${ACTIONS.CONFIGS}/${this.props.id}`)}>
                                    <Button
                                        theme='link'
                                        view='default'
                                        tone='grey'
                                        size='s'
                                        mix={{
                                            block: 'modal',
                                            elem: 'action'
                                        }}
                                        onClick={this.onModalClose}>
                                        {i18n('common', 'duplicate')}
                                    </Button>
                                </Link>
                            )}
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
                                {i18n('common', 'close')}
                            </Button>
                        </Bem>
                    ] :
                    ''
                }
            </Modal>
        );
    }
}

ConfigPreview.propTypes = {
    id: PropTypes.number,
    visible: PropTypes.bool,
    serviceId: PropTypes.string,
    configData: configType,
    schema: PropTypes.object,
    closeConfigPreview: PropTypes.func.isRequired,
    fetchSchema: PropTypes.func.isRequired
};

export default connect(state => {
    return {
        id: getConfigPreview(state).id,
        serviceId: getConfigPreview(state).serviceId,
        visible: getConfigPreview(state).visible,
        schema: getSchema(getService(state)),
        configData: getConfigPreview(state).configData
    };
}, dispatch => {
    return {
        closeConfigPreview: id => {
            dispatch(closeConfigPreview(id));
        },
        fetchSchema: labelId => {
            return dispatch(fetchSchema(labelId));
        }
    };
})(ConfigPreview);

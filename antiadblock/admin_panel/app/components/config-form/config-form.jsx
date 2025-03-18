import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import {Prompt} from 'react-router-dom';

import YAML from 'js-yaml';
import {get, has, set, unset, isEqual} from 'lodash';
import i18n from 'app/lib/i18n';

import {fetchConfigById, saveConfig} from 'app/actions/config';
import {fetchParentExpConfigs} from 'app/actions/service';
import {fetchSchema} from 'app/actions/schema';
import {getConfig, getService} from 'app/reducers/index';
import {isEmptyValue, mergeConfig, getSchemaTypeAndDefVal, clearDefaultValues} from 'app/lib/config-editor';
import {getSchema, getParentConfigsExpId} from 'app/reducers/service';

import Bem from 'app/components/bem/bem';
import ConfigYamlEditor from 'app/components/config-yaml-editor/config-yaml-editor';
import ConfigEditor from 'app/components/config-editor/config-editor';
import RadioButton from 'lego-on-react/src/components/radio-button/radio-button.react';
import Textinput from 'lego-on-react/src/components/textinput/textinput.react';
import Button from 'lego-on-react/src/components/button/button.react';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import Preloader from 'app/components/preloader/preloader';
import ConfigFormHead from './__head/config-form__head';

import './config-form.css';

import {STATUS} from 'app/enums/config';
const EDITOR = {
    FORM: 'form',
    YAML: 'yaml'
};

class ConfigForm extends React.Component {
    constructor(props) {
        super();

        this._anchors = {};

        const dataSchema = getSchemaTypeAndDefVal(props.schema || []);
        const dataParent = {
            ...dataSchema.values,
            ...props.config.dataParent
        };
        const dataCurrent = clearDefaultValues(props.config.dataCurrent, props.config.dataSettings, dataSchema.values);
        const data = mergeConfig(dataParent, dataCurrent, props.config.dataSettings, dataSchema.values);

        this.state = {
            validation: props.config.validation || {},
            isValid: true,
            comment: '',
            activeParentConfigType: props.status || props.config.status,
            activeParentConfigExpId: null,
            data: data || {},
            dataSchema,
            dataParent,
            dataCurrent,
            dataSettings: this.syncSchemaAndDataSettings(Object.keys(dataSchema.values), props.config.dataSettings),
            dataSettingsText: YAML.safeDump(props.config.dataSettings || {}, {
                lineWidth: Infinity
            }),
            text: YAML.safeDump(props.config.dataCurrent || {}, {
                lineWidth: Infinity
            }),
            editor: EDITOR.FORM
        };

        this.onSave = this.onSave.bind(this);
        this.setEditor = this.setEditor.bind(this);
        this.onErrorData = this.onErrorData.bind(this);
        this.onChangeText = this.onChangeText.bind(this);
        this.onChangeComment = this.onChangeComment.bind(this);
        this.onChangeData = this.onChangeData.bind(this);

        this.setDefaultValue = this.setDefaultValue.bind(this);
        this.isEnableDefaultValue = this.isEnableDefaultValue.bind(this);
        this.onChangeDataSettings = this.onChangeDataSettings.bind(this);
        this.onChangeParentConfigType = this.onChangeParentConfigType.bind(this);
        this.onChangeParentConfigExpId = this.onChangeParentConfigExpId.bind(this);
    }

    componentDidMount() {
        this.fetchConfigByStatus(this.props.status);

        if (!this.props.schemaLoaded) {
            this.props.fetchSchema(this.props.labelId);
        }
    }

    componentWillReceiveProps(newProps) {
        let state = {};

        if (newProps.config.loaded && newProps.schemaLoaded && (!this.props.config.loaded || !this.props.schemaLoaded)) {
            const dataSchema = getSchemaTypeAndDefVal(newProps.schema || []);
            const dataParent = {
                ...dataSchema.values,
                ...newProps.config.dataParent
            };
            const dataCurrent = clearDefaultValues(newProps.config.dataCurrent, newProps.config.dataSettings, dataSchema.values);
            const data = mergeConfig(dataParent, dataCurrent, newProps.config.dataSettings, dataSchema.values);

            state = {
                data,
                dataSchema,
                dataParent,
                dataCurrent,
                dataSettings: this.syncSchemaAndDataSettings(Object.keys(dataSchema.values), newProps.config.dataSettings),
                dataSettingsText: YAML.safeDump(newProps.config.dataSettings || {}, {
                    lineWidth: Infinity
                }),
                text: YAML.safeDump(newProps.config.dataCurrent || {}, {
                    lineWidth: Infinity
                }),
                activeParentConfigType: this.state.activeParentConfigType ? this.state.activeParentConfigType : (newProps.config.status || STATUS.ACTIVE)
            };
        }

        this.setState({
            ...state,
            validation: newProps.config.validation,
            isValid: this.isValid(newProps.config.validation)
        });
    }

    syncSchemaAndDataSettings(schemaKeys = [], ds = {}) {
        return schemaKeys.reduce((DS, key) => {
            set(DS, [key, 'UNSET'], get(ds, [key, 'UNSET'], false));

            return DS;
        }, {});
    }

    getItemDefaultData(key) {
        return this.state.dataSchema.values[key];
    }

    getItemType(key) {
        return this.state.dataSchema.types[key];
    }

    getAnchorRefSetter(name) {
        return ref => {
            this._anchors[name] = ref;
        };
    }

    isChanged() {
        // Сравниваем данные в props и в state
        const prevData = JSON.stringify((this.props.config && this.props.config.dataCurrent) || {});
        const isDataChanged = (JSON.stringify(this.state.dataCurrent) !== prevData);
        const isCommentChanged = this.state.comment !== '';

        return isDataChanged || isCommentChanged;
    }

    yamlToData() {
        const {
            dataSettingsText,
            text
        } = this.state;
        let isValid = true,
            dataSettings = {},
            dataCurrent = {};

        try {
            dataSettings = YAML.safeLoad(dataSettingsText);
            dataCurrent = YAML.safeLoad(text);
        } catch (e) {
            isValid = false;
        }

        if (isValid) {
            dataSettings = Object.keys(dataSettings).reduce((DS, key) => {
                const dsValue = get(dataSettings, [key, 'UNSET']);

                if (this.getItemDefaultData(key) !== undefined && (typeof dsValue === 'boolean')) {
                    set(DS, [key, 'UNSET'], dsValue);
                }

                return DS;
            }, {});

            dataCurrent = Object.keys(dataCurrent).reduce((DC, key) => {
                if (this.getItemDefaultData(key) !== undefined && !get(dataSettings, [key, 'UNSET']) && !isEqual(this.getItemDefaultData(key), dataCurrent[key])) {
                    DC[key] = dataCurrent[key];
                }

                return DC;
            }, {});
        }

        return {
            isValid,
            dataCurrent,
            dataSettings
        };
    }

    onSave() {
        const {
            editor,
            comment,
            dataCurrent,
            dataSettings
        } = this.state;
        const {
            saveConfig,
            labelId,
            config,
            serviceId
        } = this.props;

        if (editor === EDITOR.YAML) {
            const newState = this.yamlToData();

            if (newState.isValid) {
                this.setState({
                    isValid: true
                });

                saveConfig(
                    serviceId,
                    labelId,
                    config.id,
                    newState.dataCurrent,
                    newState.dataSettings,
                    comment
                );
            } else {
                this.setState({
                    isValid: false
                });
            }
        } else {
            saveConfig(
                serviceId,
                labelId,
                config.id,
                dataCurrent,
                dataSettings,
                comment
            );
        }
    }

    setEditor(e) {
        const editor = e.target.value;
        const {
            dataSettings,
            dataCurrent,
            dataParent,
            dataSchema
        } = this.state;
        let newState = {};

        switch (editor) {
            case EDITOR.FORM:
                newState = this.yamlToData();

                if (newState.isValid) {
                    newState = {
                        ...newState,
                        editor,
                        data: mergeConfig(dataParent, newState.dataCurrent, newState.dataSettings, dataSchema.values)
                    };
                } else {
                    newState = {
                        isValid: false
                    };
                }
                break;
            case EDITOR.YAML:
                newState = {
                    editor,
                    text: YAML.safeDump(dataCurrent, {
                        lineWidth: Infinity
                    }),
                    dataSettingsText: YAML.safeDump(Object.keys(dataSettings).reduce((DS, key) => {
                        if (dataCurrent[key] !== undefined || get(dataSettings, [key, 'UNSET'])) {
                            DS[key] = dataSettings[key];
                        }

                        return DS;
                    }, {}), {
                        lineWidth: Infinity
                    })
                };
                break;
            default:
                break;
        }

        this.setState({
            ...newState
        });
    }

    onChangeComment(comment) {
        const {
            validation
        } = this.state;
        let newState = {};

        if (validation.comment.length) {
            newState = {
                validation: {
                    data: validation.data,
                    comment: []
                }
            };

            if (this.isValid(newState.validation)) {
                newState.isValid = true;
            }
        }

        this.setState({
            ...newState,
            comment
        });
    }

    onChangeDataSettings(dataSettingsText) {
        const {
            dataSettings,
            text
        } = this.state;
        let isValidDS = true,
            isValidText = true,
            newDataSettings = {},
            newDataCurr = {},
            newState = {};

        try {
            newDataSettings = YAML.safeLoad(dataSettingsText);
        } catch (e) {
            isValidDS = false;
        }

        if (isValidDS) {
            const prepDataSettings = Object.keys(newDataSettings).reduce((ds, key) => {
                if (this.getItemDefaultData(key) !== undefined) {
                    ds[key] = newDataSettings[key];
                }
                return ds;
            }, {});
            try {
                newDataCurr = YAML.safeLoad(text);
            } catch (e) {
                isValidText = false;
            }

            if (!isEqual(prepDataSettings, dataSettings)) {
                newState = {
                    dataSettings: prepDataSettings
                };
                if (isValidText) {
                    Object.keys(prepDataSettings).forEach(key => {
                        if (get(prepDataSettings, [key, 'UNSET']) === true) {
                            newDataCurr[key] = this.getItemDefaultData(key);
                        }
                    });

                    newState = {
                        ...newState,
                        text: YAML.safeDump(newDataCurr || {}, {
                            lineWidth: Infinity
                        })
                    };
                }
            }
        }

        this.setState({
            ...newState,
            dataSettingsText
        });
    }

    onChangeText(text) {
        let {
            dataCurrent,
            validation,
            dataSettingsText
        } = this.state,
            isValid = true,
            isValidDS = true,
            newState = {},
            newDataCurrent = {},
            newDataSettings = {};

        try {
            newDataCurrent = YAML.safeLoad(text);
        } catch (e) {
            isValid = false;
        }

        if (!validation.data.length) {
            newState = {
                validation: {
                    comment: validation.comment,
                    data: []
                }
            };
        }

        if (isValid) {
            try {
                newDataSettings = YAML.safeLoad(dataSettingsText);
            } catch (e) {
                isValidDS = false;
            }

            if (isValidDS) {
                let isChanged = false;

                // смотрим не добавилось или не поменялось значение у поля
                Object.keys(newDataCurrent).forEach(key => {
                    if (this.getItemDefaultData(key) && !isEqual(newDataCurrent[key], dataCurrent[key])) {
                        set(newDataSettings, [key, 'UNSET'], false);
                        isChanged = true;
                    }
                });
                // смотрим не удалили ли поле
                Object.keys(dataCurrent).forEach(key => {
                    if (this.getItemDefaultData(key) && dataCurrent[key] && newDataCurrent[key] === undefined) {
                        set(newDataSettings, [key, 'UNSET'], false);
                        isChanged = true;
                    }
                });

                if (isChanged) {
                    newState = {
                        ...newState,
                        dataCurrent: newDataCurrent,
                        dataSettingsText: YAML.safeDump(newDataSettings || {}, {
                            lineWidth: Infinity
                        })
                    };
                }
            }
        }

        this.setState({
            ...newState,
            isValid,
            text
        });
    }

    onErrorData(name, value) {
        const {
            validation
        } = this.state;
        let newState = {};

        if (!has(validation.data, name)) {
            validation.data[name] = value;
            newState.validation = validation;
        }

        this.setState({
            isValid: false,
            ...newState
        });
    }

    setDefaultValue(name) {
        const path = [name, 'UNSET'];

        return () => {
            const {
                dataSettings,
                data,
                dataCurrent,
                validation,
                dataParent
            } = this.state;
            const isEnable = !get(dataSettings, path);
            let newState = {};

            if (isEnable) {
                const defaultValue = this.getItemDefaultData(name);

                if (has(validation.data, name)) {
                    unset(validation.data, name);
                    newState = {
                        validation,
                        isValid: this.isValid(validation)
                    };
                }

                unset(dataCurrent, name);
                set(data, name, defaultValue);
                newState.dataCurrent = dataCurrent;
                newState.data = data;
            } else {
                set(data, name, get(dataParent, name));
                newState.data = data;
            }
            set(dataSettings, path, isEnable);
            newState.dataSettings = dataSettings;

            this.setState({
                ...newState
            });
        };
    }

    isEnableDefaultValue(name) {
        const path = [name, 'UNSET'];

        return Boolean(get(this.state.dataSettings, path));
    }

    isValid(validation) {
        return !Object.keys(validation.data).length && !validation.comment.length;
    }

    onChangeData(name) {
        return value => {
            const {
                dataCurrent,
                dataParent,
                data,
                dataSettings,
                validation
            } = this.state;
            const {
                schema
            } = this.props;
            const type = this.getItemType(name);
            let newState = {};

            if (isEmptyValue(type, value, schema, name) && !this.isEnableDefaultValue(name)) {
                if (has(dataCurrent, name)) {
                    unset(dataCurrent, name);
                    newState.dataCurrent = dataCurrent;
                }
                set(data, name, get(dataParent, name));
                newState.data = data;
            } else {
                set(dataSettings, [name, 'UNSET'], false);
                set(dataCurrent, name, value);
                set(data, name, value);
                newState = {
                    dataSettings,
                    dataCurrent,
                    data
                };
            }

            if (has(validation.data, name)) {
                unset(validation.data, name);

                newState.validation = validation;
                newState.isValid = this.isValid(validation);
            }

            this.setState({
                ...newState
            });
        };
    }

    fetchConfigByStatus(status) {
        switch (status) {
            case STATUS.ACTIVE:
                this.props.fetchConfigById(this.props.configId);
                break;
            case STATUS.TEST:
                this.props.fetchConfigById(this.props.configId, 'test');
                break;
            case STATUS.EXPERIMENT:
                this.props.fetchParentExpConfigs(this.props.labelId);
                break;
            default:
                this.props.fetchConfigById(this.props.configId);
                break;
        }
    }

    onChangeParentConfigType(value) {
        this.fetchConfigByStatus(value[0]);

        this.setState({
            activeParentConfigType: value[0]
        });
    }

    onChangeParentConfigExpId(value) {
        this.props.fetchConfigById(this.props.configId, undefined, value[0]);

        this.setState({
            activeParentConfigExpId: value[0]
        });
    }

    render() {
        if (!this.props.schemaLoaded || !this.props.config.loaded) {
            return (
                <Preloader />
            );
        }

        return (
            <Bem block='config-form'>
                <Prompt
                    when={this.isChanged()}
                    message={i18n('service-page', 'save-changes-message')} />
                <Bem
                    key='sticky'
                    block='config-form'
                    elem='sticky'>
                    <Bem
                        key='radio'
                        block='config-form'
                        elem='radio'>
                        <RadioButton
                            name='tone'
                            theme='normal'
                            size='s'
                            view='default'
                            tone='default'
                            value={this.state.editor}
                            disabled={!this.state.isValid}
                            onChange={this.setEditor} >
                            <RadioButton.Radio value={EDITOR.FORM}>
                                {i18n('service-page', 'form-editor')}
                            </RadioButton.Radio>
                            <RadioButton.Radio value={EDITOR.YAML}>
                                {i18n('service-page', 'yaml-editor')}
                            </RadioButton.Radio>
                        </RadioButton>
                    </Bem>
                </Bem>
                <Bem
                    key='main'
                    block='config-form'
                    elem='main'>
                    <ConfigFormHead
                        activeParentConfigType={this.state.activeParentConfigType}
                        activeParentConfigExpId={this.state.activeParentConfigExpId}
                        onChangeParentConfigExpId={this.onChangeParentConfigExpId}
                        onChangeParentConfigType={this.onChangeParentConfigType}
                        parentConfigsExpId={this.props.parentConfigsExpId} />
                    <Bem
                        key='form'
                        block='config-form'
                        mods={{
                            [this.state.editor]: true
                        }}
                        elem='form'>
                        {this.state.editor === EDITOR.FORM ?
                            <ConfigEditor
                                dataCurrent={this.state.dataCurrent}
                                data={this.state.data}
                                validation={this.state.validation.data}
                                schema={this.props.schema}
                                onChange={this.onChangeData}
                                onError={this.onErrorData}
                                dataSettings={this.state.dataSettings}
                                setDefaultValue={this.setDefaultValue}
                                isEnableDefaultValue={this.isEnableDefaultValue} /> :
                            <ConfigYamlEditor
                                data={this.state.text}
                                dataSettings={this.state.dataSettingsText}
                                comment={this.state.comment}
                                keywords={Object.keys(this.props.schema)}
                                validation={this.state.validation.data}
                                onChange={this.onChangeText}
                                onChangeDataSettings={this.onChangeDataSettings} />}
                    </Bem>
                </Bem>
                <Bem
                    key='actions'
                    block='config-form'
                    elem='actions'>
                    <Bem
                        key='comment'
                        block='config-form'
                        elem='comment'>
                        <Bem
                            key='input'
                            block='config-form'
                            elem='input'
                            ref={this.getAnchorRefSetter('comment')}>
                            <Textinput
                                theme='normal'
                                tone='grey'
                                view='default'
                                size='m'
                                to='bottom-left'
                                placeholder={i18n('service-page', 'config-form-comment-title')}
                                text={this.state.comment}
                                onChange={this.onChangeComment} />
                        </Bem>
                        <Tooltip
                            anchor={this._anchors.comment}
                            view='classic'
                            tone='default'
                            theme='error'
                            to={['top']}
                            mainOffset={10}
                            mix={{
                                block: 'config-form',
                                elem: 'tooltip'
                            }}
                            visible={Boolean(this.state.validation.comment && this.state.validation.comment.length)}>
                            {this.state.validation.comment
                                .map(item => item.message)
                                .join('\n')}
                        </Tooltip>
                    </Bem>
                    <Bem
                        key='save'
                        block='config-form'
                        elem='save'>
                        <Button
                            view='default'
                            tone='grey'
                            type='action'
                            size='s'
                            theme='action'
                            disabled={!this.state.isValid || (this.state.activeParentConfigType === STATUS.EXPERIMENT && !this.state.activeParentConfigExpId)}
                            progress={this.props.config.saving}
                            onClick={this.onSave}>
                            {i18n('common', 'save')}
                        </Button>
                    </Bem>
                </Bem>
            </Bem>
        );
    }
}

ConfigForm.propTypes = {
    serviceId: PropTypes.string.isRequired,
    configId: PropTypes.string.isRequired,
    labelId: PropTypes.string.isRequired,
    schema: PropTypes.array,
    fetchSchema: PropTypes.func,
    fetchConfigById: PropTypes.func,
    fetchParentExpConfigs: PropTypes.func,
    saveConfig: PropTypes.func,
    config: PropTypes.object,
    schemaLoaded: PropTypes.bool,
    parentConfigsExpId: PropTypes.object,
    status: PropTypes.string
};

export default connect(state => {
    const schema = getSchema(getService(state));
    const parentConfigsExpId = getParentConfigsExpId(getService(state));

    return {
        schema: schema.schema,
        schemaLoaded: schema.loaded,
        config: getConfig(state),
        parentConfigsExpId
    };
}, dispatch => {
    return {
        fetchSchema: labelId => {
            return dispatch(fetchSchema(labelId));
        },
        fetchConfigById: (configId, status, expId) => {
            return dispatch(fetchConfigById(configId, status, expId));
        },
        // eslint-disable-next-line max-params
        saveConfig: (serviceId, labelId, parentId, comment, data, dataSettings) => {
            return dispatch(saveConfig(serviceId, labelId, parentId, comment, data, dataSettings));
        },
        fetchParentExpConfigs: labelId => {
            return dispatch(fetchParentExpConfigs(labelId));
        }
    };
})(ConfigForm);

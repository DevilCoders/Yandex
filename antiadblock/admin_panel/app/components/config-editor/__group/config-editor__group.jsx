import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';

import i18n from 'app/lib/i18n';
import {getConfig} from 'app/lib/config-editor';
import isEqual from 'lodash/isEqual';

import './config-editor__group.css';

class ConfigGroupEditor extends React.Component {
    constructor() {
        super();

        this.state = {
            groupIsVisible: false
        };

        this.onChangeVisible = this.onChangeVisible.bind(this);
        this.hasErrors = this.hasErrors.bind(this);
    }

    shouldComponentUpdate(nextProps, nextState) {
        return (
            nextState.groupIsVisible !== this.state.groupIsVisible ||
            !isEqual(nextProps.data, this.props.data) ||
            !isEqual(nextProps.dataSettings, this.props.dataSettings) ||
            !isEqual(nextProps.dataCurrent, this.props.dataCurrent) ||
            !isEqual(nextProps.validation, this.props.validation)
        );
    }

    onChangeVisible() {
        this.setState(state => ({
            groupIsVisible: !state.groupIsVisible
        }));
    }

    hasErrors() {
        const {
            items,
            validation
        } = this.props;

        return items.some(item => (
            validation[item.key.name]
        ));
    }

    isFieldFromParentConfig(title, key) {
        return title && !this.props.isEnableItemConfig(key) && !this.props.isEnableDefaultValue(key);
    }

    getEditor(key, config, data, validation) {
        const {
            readOnly,
            onChange,
            onError,
            setEditorRef,
            isEnableDefaultValue,
            setDefaultValue
        } = this.props;
        const path = `data.${key}`;
        const editorConfig = getConfig(config.type_schema);
        const Constructor = editorConfig.constructor;

        let value = config.default;

        if (data[key] !== undefined) {
            value = data[key];
        }

        return (
            <Constructor
                {...editorConfig.props}
                path={path}
                yamlKey={key}
                title={editorConfig.props.title || key}
                hint={editorConfig.props.hint || config.hint}
                value={value}
                validation={validation[key]}
                readOnly={readOnly || config.read_only}
                config={config}
                onChange={onChange ? onChange(key) : () => {}}
                onError={onError(key)}
                setEditorRef={setEditorRef}
                isFromParentConfig={this.isFieldFromParentConfig(editorConfig.props.title || key, key)}
                isDefaultValue={isEnableDefaultValue(key)}
                setDefaultValue={setDefaultValue ? setDefaultValue(key) : () => {}} />
        );
    }

    renderField() {
        const {
            items,
            data,
            validation
        } = this.props;

        return (
            items.map(field => {
                return (
                    <Bem
                        key={field.key.name}
                        block='config-group-editor'
                        elem='field'>
                        {this.getEditor(field.key.name, field.key, data, validation)}
                    </Bem>
                );
            })
        );
    }

    render() {
        const {
            title
        } = this.props;
        const {
            groupIsVisible
        } = this.state;

        return (
            <Bem
                key={title}
                block='config-group-editor'>
                <Bem
                    block='config-group-editor'
                    elem='title'>
                    <Button
                        view='classic'
                        tone='grey'
                        type='check'
                        size='m'
                        theme='normal'
                        width='max'
                        mix={{
                            block: 'config-group-editor',
                            elem: 'title-button',
                            mods: {
                                checked: groupIsVisible,
                                'has-errors': this.hasErrors()
                            }
                        }}
                        checked={groupIsVisible}
                        iconRight={{
                            mods: {
                                type: 'arrow',
                                direction: groupIsVisible ? 'down' : 'right'
                            }
                        }}
                        title={title}
                        onClick={this.onChangeVisible}>
                        {i18n('config-editor-group', title)}
                    </Button>
                </Bem>
                <Bem
                    block='config-group-editor'
                    elem='fields'
                    mods={{
                        visible: groupIsVisible
                    }}>
                    {groupIsVisible && this.renderField()}
                </Bem>
            </Bem>
        );
    }
}

ConfigGroupEditor.contextTypes = {
    scope: PropTypes.object
};

ConfigGroupEditor.propTypes = {
    title: PropTypes.string.isRequired,
    items: PropTypes.array.isRequired,
    data: PropTypes.object.isRequired,
    validation: PropTypes.object.isRequired,
    readOnly: PropTypes.bool,
    onChange: PropTypes.func,
    onError: PropTypes.func.isRequired,
    setDefaultValue: PropTypes.func,
    isEnableItemConfig: PropTypes.func,
    isEnableDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func,
    dataSettings: PropTypes.object,
    dataCurrent: PropTypes.object
};

export default ConfigGroupEditor;

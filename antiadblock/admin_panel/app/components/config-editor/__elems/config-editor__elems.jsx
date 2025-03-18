import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import ConfigArrayEditor from '../__array/config-editor__array';
import ConfigTextEditor from '../__text/config-editor__text';

import i18n from 'app/lib/i18n';

import './config-editor__elems.css';

const children = [
    'id',
    'class',
    'data-id',
    'aria-label'
];

class ConfigElemsEditor extends React.PureComponent {
    constructor() {
        super();

        this.onChange = this.onChange.bind(this);
    }

    onChange(name) {
        return (value, path) => {
            const nextValue = {
                ...this.props.value,
                [name]: value
            };

            // Удаляем пустые значения
            if (!value || !value.length) {
                delete nextValue[name];
            }

            this.props.onChange(nextValue, path);
        };
    }

    render() {
        const value = this.props.value || {};
        return (
            <Bem block='config-elems-editor'>
                {children.map(key => {
                    const childValue = value[key];

                    // Не рисуем пустые редакторы в read-only режиме
                    if (this.props.readOnly && !(childValue && childValue.length)) {
                        return null;
                    }

                    return (
                        <Bem
                            key={key}
                            block='config-elems-editor'
                            elem='array'>
                            <ConfigArrayEditor
                                {...this.props}
                                yamlKey={this.props.yamlKey}
                                validation={this.props.validation}
                                path={`${this.props.path}.${key}`}
                                title={`${i18n('titles', this.props.title)} ${key.toUpperCase()}`}
                                Item={ConfigTextEditor}
                                value={value[key]}
                                onChange={this.onChange(key)} />
                        </Bem>
                    );
                })}
            </Bem>
        );
    }
}

ConfigElemsEditor.propTypes = {
    title: PropTypes.string,
    yamlKey: PropTypes.string,
    hint: PropTypes.string,
    path: PropTypes.string,
    placeholder: PropTypes.any,
    value: PropTypes.object,
    validation: PropTypes.array,
    readOnly: PropTypes.bool,
    onChange: PropTypes.func,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigElemsEditor;

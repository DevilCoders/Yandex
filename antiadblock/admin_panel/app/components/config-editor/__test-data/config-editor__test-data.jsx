import React from 'react';
import PropTypes from 'prop-types';

import {set, get} from 'lodash';

import Bem from 'app/components/bem/bem';

import ConfigArrayEditor from '../__array/config-editor__array';
import ConfigTextEditor from '../__text/config-editor__text';

import './config-editor__test-data.css';

const children = [
    'valid',
    'invalid',
    'paths_valid',
    'paths_invalid'
];

class ConfigTestDataEditor extends React.Component {
    constructor() {
        super();

        this.getOnChangeHandler = this.getOnChangeHandler.bind(this);
    }

    getOnChangeHandler(name) {
        return (value, path) => {
            const newValue = {
                ...this.props.value
            };
            set(newValue, name, value);

            // Удаляем пустые значения
            if (!value || !value.length) {
                delete get(newValue, name);
            }

            this.props.onChange(newValue, path);
        };
    }

    render() {
        const value = this.props.value || {};
        const validation = this.props.validation;

        return (
            <Bem block='config-elems-editor'>
                {children.map(key => {
                    const pathsExist = !key.indexOf('paths_') && value.paths;
                    const path = pathsExist ? key.replace('_', '.') : `${key}_domains`;
                    const childValue = get(value, path);

                    // Не рисуем пустые редакторы в read-only режиме
                    if (this.props.readOnly && !(childValue && childValue.length)) {
                        return null;
                    }

                    return (
                        <Bem
                            key={key}
                            block='config-test-data-editor'
                            elem='array'>
                            <ConfigArrayEditor
                                {...this.props}
                                hint={this.props.hint[`${key}_hint`]}
                                title={this.props.title[`${key}_title`]}
                                yamlKey={this.props.yamlKey}
                                path={`${this.props.path}.${path}`}
                                placeholder={this.props.placeholder[`${key}_placeholder`]}
                                validation={validation}
                                Item={ConfigTextEditor}
                                value={childValue}
                                onChange={this.getOnChangeHandler(path)} />
                        </Bem>
                    );
                })}
            </Bem>
        );
    }
}

ConfigTestDataEditor.propTypes = {
    title: PropTypes.object,
    yamlKey: PropTypes.string,
    hint: PropTypes.object,
    path: PropTypes.string,
    placeholder: PropTypes.any,
    value: PropTypes.object,
    validation: PropTypes.array,
    readOnly: PropTypes.bool,
    onChange: PropTypes.func
};

export default ConfigTestDataEditor;

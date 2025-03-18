import React from 'react';
import PropTypes from 'prop-types';

import isEqual from 'lodash/isEqual';

import Bem from 'app/components/bem/bem';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';
import Select from 'lego-on-react/src/components/select/select.react';

import i18n from 'app/lib/i18n';

import './config-editor__enum.css';

class ConfigEnumEditor extends React.PureComponent {
    constructor() {
        super();

        this.onChange = this.onChange.bind(this);
    }

    onChange(value) {
        this.props.onChange(this.props.enum[value], this.props.path);
    }

    validate() {
        let answer = [];

        if (this.props.validation) {
            for (let i = 0; i < this.props.validation.length; i++) {
                const error = this.props.validation[i];
                const path = error.path.join('.');

                if (path.includes(this.props.path)) {
                    answer.push(error.message);

                    break;
                }
            }
        }

        return answer.join('\n');
    }

    render() {
        const validation = this.validate();

        // В Lego в value в качестве значения после обновления нельзя передавать массив
        // поэтому в качестве значения используется ключ. Значение берется в onChange
        const selectKey = Object.keys(this.props.enum).filter(key => {
            return isEqual(this.props.enum[key], this.props.value);
        })[0];

        const mods = {
            'has-errors': Boolean(validation),
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-enum-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        block='config-enum-editor'
                        elem='title'>
                        <ConfigEditorTitle
                            title={this.props.title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint}
                            readOnly={this.props.readOnly}
                            setDefaultValue={this.props.setDefaultValue}
                            isDefaultValue={this.props.isDefaultValue} />
                    </Bem>}
                <Bem
                    block='config-enum-editor'
                    elem='body'>
                    <Select
                        theme='pseudo'
                        view='default'
                        tone='grey'
                        size='m'
                        width='max'
                        type={this.props.multi ? 'check' : 'radio'}
                        val={selectKey}
                        onChange={this.onChange}
                        popup={{
                            scope: this.context.scope && this.context.scope.dom
                        }}
                        disabled={this.props.readOnly} >
                        {Object.keys(this.props.enum).map(key => (
                            <Select.Item
                                key={key}
                                val={key}>
                                {i18n('placeholders', key) || key}
                            </Select.Item>
                        ))}
                    </Select>
                </Bem>
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigEnumEditor.contextTypes = {
    scope: PropTypes.object
};

ConfigEnumEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    validation: PropTypes.array,
    enum: PropTypes.object.isRequired,
    yamlKey: PropTypes.string,
    title: PropTypes.string,
    hint: PropTypes.string,
    value: PropTypes.any,
    multi: PropTypes.bool,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigEnumEditor;

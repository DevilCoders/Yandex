import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';

import './config-editor__tuple.css';

// TODO Пока tuple используется только в object можно размечать редакторы как key-value
const names = ['key', 'value'];

class ConfigTupleEditor extends React.PureComponent {
    constructor() {
        super();

        this.onChange = this.onChange.bind(this);
    }

    validate() {
        let answer = [];

        if (this.props.validation) {
            for (let i = 0; i < this.props.validation.length; i++) {
                const error = this.props.validation[i];
                const path = error.path.join('.');

                if (this.props.path === path) {
                    answer.push(error.message);

                    break;
                }
            }
        }

        return answer.join('\n');
    }

    onChange(index) {
        return value => {
            if (this.props.onChange) {
                const newValue = [...(this.props.value || [])];
                newValue[index] = value;
                this.props.onChange(newValue);
            }
        };
    }

    render() {
        const value = this.props.value || [];
        const validation = this.validate();
        const placeholder = this.props.placeholder || [];
        const mods = {
            'has-errors': Boolean(validation),
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-tuple-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        key='title'
                        block='config-tuple-editor'
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
                    block='config-tuple-editor'
                    elem='body'>
                    {this.props.items.map((Item, index) => {
                        return (
                            <Bem
                                key={names[index]}
                                block='config-tuple-editor'
                                elem='item'>
                                <Item
                                    setEditorRef={this.props.setEditorRef}
                                    value={value[index]}
                                    path={this.props.path}
                                    placeholder={placeholder[index]}
                                    onChange={this.onChange(index)}
                                    readOnly={this.props.readOnly} />
                            </Bem>
                        );
                    })}
                </Bem>
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigTupleEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    title: PropTypes.string,
    yamlKey: PropTypes.string,
    hint: PropTypes.string,
    value: PropTypes.array,
    validation: PropTypes.array,
    placeholder: PropTypes.any,
    readOnly: PropTypes.bool,
    items: PropTypes.array,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigTupleEditor;

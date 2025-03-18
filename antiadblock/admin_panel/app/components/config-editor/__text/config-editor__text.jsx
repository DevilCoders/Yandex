import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';
import TextInput from 'lego-on-react/src/components/textinput/textinput.react';

import i18n from 'app/lib/i18n';

import './config-editor__text.css';

class ConfigTextEditor extends React.PureComponent {
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

    onChange(value) {
        if (this.props.onChange) {
            this.props.onChange(value, this.props.path);
        }
    }

    render() {
        const validation = this.validate();
        const mods = {
            'has-errors': Boolean(validation),
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-text-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        block='config-text-editor'
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
                    block='config-text-editor'
                    elem='body'>
                    <TextInput
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='m'
                        type={this.props.type}
                        text={this.props.value ? String(this.props.value) : ''}
                        placeholder={i18n('placeholders', this.props.placeholder) || this.props.placeholder}
                        onChange={this.onChange}
                        disabled={this.props.readOnly} />
                </Bem>
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigTextEditor.propTypes = {
    title: PropTypes.string,
    hint: PropTypes.string,
    yamlKey: PropTypes.string,
    path: PropTypes.string,
    type: PropTypes.string,
    value: PropTypes.any,
    validation: PropTypes.array,
    placeholder: PropTypes.string,
    readOnly: PropTypes.bool,
    onChange: PropTypes.func,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigTextEditor;

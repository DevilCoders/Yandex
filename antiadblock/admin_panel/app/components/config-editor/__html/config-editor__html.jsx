import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import MonacoEditor from 'app/components/monaco-editor/monaco-editor';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';

import './config-editor__html.css';

class ConfigHtmlEditor extends React.PureComponent {
    constructor() {
        super();

        this.onChange = this.onChange.bind(this);
    }

    onChange(value) {
        this.props.onChange(value, this.props.path);
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

    render() {
        const validation = this.validate();
        const mods = {
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-html-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        block='config-html-editor'
                        elem='title'
                        key='title'>
                        <ConfigEditorTitle
                            title={this.props.title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint}
                            readOnly={this.props.readOnly}
                            setDefaultValue={this.props.setDefaultValue}
                            isDefaultValue={this.props.isDefaultValue} />
                    </Bem>}
                <Bem
                    block='config-html-editor'
                    elem='body'>
                    <MonacoEditor
                        language='html'
                        height='100'
                        options={{
                            scrollBeyondLastLine: false,
                            readOnly: this.props.readOnly,
                            minimap: {
                                enabled: false
                            }
                        }}
                        value={this.props.value || ''}
                        onChange={this.onChange} />
                </Bem>
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigHtmlEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    validation: PropTypes.array,
    title: PropTypes.string,
    yamlKey: PropTypes.string,
    hint: PropTypes.string,
    // placeholder: PropTypes.any,
    value: PropTypes.string,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigHtmlEditor;

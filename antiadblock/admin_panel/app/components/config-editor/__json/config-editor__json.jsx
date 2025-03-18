import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import MonacoEditor from 'app/components/monaco-editor/monaco-editor';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';

import './config-editor__json.css';

class ConfigJsonEditor extends React.Component {
    constructor(props) {
        super();

        let value = '{}';

        try {
            value = JSON.stringify(props.value || {}, null, 2);
        } catch (e) {}

        this.state = {
            value
        };

        this.onChange = this.onChange.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        // TODO Неудачная проверка
        if (this.state.value === '{}' || !nextProps.value) {
            let value = '';

            try {
                value = JSON.stringify(nextProps.value || {}, null, 2);
            } catch (e) {}
            this.setState({
                value
            });
        }
    }

    onChange(value) {
        this.setState({
            value
        });

        let isValid = true,
            obj = null;

        try {
            obj = JSON.parse(value);
        } catch (e) {
            isValid = false;
        }

        if (!isValid) {
            this.props.onError([{
                message: 'Invalid json',
                path: this.props.path.split('.')
            }]);
        } else {
            this.props.onChange(obj, this.props.path);
        }
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
        const value = this.state.value;
        const validation = this.validate();
        const mods = {
            'from-parent': this.props.isFromParentConfig,
            'has-errors': Boolean(validation)
        };

        return (
            <Bem
                block='config-json-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        block='config-json-editor'
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
                    block='config-json-editor'
                    elem='body'>
                    <MonacoEditor
                        language='json'
                        height='200'
                        options={{
                            scrollBeyondLastLine: false,
                            readOnly: this.props.readOnly,
                            minimap: {
                                enabled: false
                            }
                        }}
                        value={value}
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

ConfigJsonEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    onError: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    validation: PropTypes.array,
    yamlKey: PropTypes.string,
    title: PropTypes.string,
    hint: PropTypes.string,
    // placeholder: PropTypes.any,
    value: PropTypes.object,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigJsonEditor;

import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEnumEditor from 'app/components/config-editor/__enum/config-editor__enum';
import ConfigTextEditor from 'app/components/config-editor/__text/config-editor__text';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';

import './config-editor__link.css';

const enums = {
    HEAD: 'head',
    POST: 'post',
    GET: 'get',
    SCRIPT: 'script',
    IMG: 'img'
};

class ConfigLinkEditor extends React.PureComponent {
    constructor() {
        super();

        // props.value => {type: 'get', src: 'sdsdsd'}
        this.onChange = this.onChange.bind(this);
    }

    onChange(name) {
        const defaultValue = {
            src: '',
            type: 'get'
        };

        return value => {
            this.props.onChange({
                ...defaultValue,
                ...this.props.value,
                [name]: value
            }, this.props.path);
        };
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
        const value = this.props.value || {};
        const type = value.type || 'get';
        const src = value.src || '';
        const validation = this.validate();
        const mods = {
            'has-errors': Boolean(validation),
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-link-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        key='title'
                        block='config-link-editor'
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
                    block='config-link-editor'
                    elem='body'>
                    <Bem
                        key='type'
                        block='config-link-editor'
                        elem='type'>
                        <ConfigEnumEditor
                            value={type}
                            enum={enums}
                            setEditorRef={this.props.setEditorRef}
                            path={this.props.path}
                            validation={[]}
                            onChange={this.onChange('type')}
                            readOnly={this.props.readOnly} />
                    </Bem>
                    <Bem
                        key='text'
                        block='config-link-editor'
                        elem='text'>
                        <ConfigTextEditor
                            value={src}
                            setEditorRef={this.props.setEditorRef}
                            placeholder={this.props.placeholder}
                            onChange={this.onChange('src')}
                            readOnly={this.props.readOnly} />
                    </Bem>
                </Bem>
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigLinkEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    validation: PropTypes.array,
    path: PropTypes.string.isRequired,
    yamlKey: PropTypes.string,
    title: PropTypes.string,
    hint: PropTypes.string,
    value: PropTypes.object,
    readOnly: PropTypes.bool,
    placeholder: PropTypes.string,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigLinkEditor;

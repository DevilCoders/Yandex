import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import ConfigArrayEditor from '../__array/config-editor__array';
import ConfigObjectEditor from '../__object/config-editor__object';
import ConfigTextEditor from '../__text/config-editor__text';
import ConfigTupleEditor from '../__tuple/config-editor__tuple';

import './config-editor__crypt.css';

class ConfigCryptEditor extends React.PureComponent {
    constructor(props) {
        super();

        this.state = this.proceed(props.value);

        this.onChangeArray = this.onChangeArray.bind(this);
        this.onChangeObject = this.onChangeObject.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        // Синхронизируем value только если текущее значение пусто
        if (!this.state.crypt.length && Object.keys(this.state.replace)) {
            this.setState(this.proceed(nextProps.value));
        }
    }

    proceed(value) {
        const result = {
            crypt: [],
            replace: {}
        };

        if (value) {
            Object.keys(value).forEach(key => {
                if (value[key] === null) {
                    result.crypt.push(key);
                } else {
                    result.replace[key] = value[key];
                }
            });
        }

        return result;
    }

    buildObject(value) {
        let answer = {};

        value.crypt.forEach(key => {
            if (key) {
                answer[key] = null;
            }
        });

        Object.assign(answer, value.replace);

        return answer;
    }

    onChangeArray(value, path) {
        this.setState({
            crypt: value
        });

        this.props.onChange(this.buildObject({
            ...this.state,
            crypt: value
        }), path);
    }

    onChangeObject(value, path) {
        this.setState({
            replace: value
        });

        this.props.onChange(this.buildObject({
            ...this.state,
            replace: value
        }), path);
    }

    arrayKey(value, index) {
        if (value) {
            return value;
        }

        return index;
    }

    render() {
        const isCryptBlockVisible = !this.props.readOnly || Boolean(this.state.crypt && this.state.crypt.length);
        const isReplaceBlockVisible = (!this.props.readOnly || Boolean(this.state.replace && Object.keys(this.state.replace).length));

        return (
            <Bem block='config-crypt-editor'>
                {isCryptBlockVisible &&
                    <Bem
                        block='config-crypt-editor'
                        elem='array'>
                        <ConfigArrayEditor
                            {...this.props}
                            title={this.props.title.crypt_title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint.crypt_hint}
                            childProps={{
                                placeholder: this.props.placeholder.crypt_placeholder
                            }}
                            keyFunction={this.arrayKey}
                            Item={ConfigTextEditor}
                            value={this.state.crypt}
                            onChange={this.onChangeArray} />
                    </Bem>
                }

                {isReplaceBlockVisible &&
                    <Bem
                        block='config-crypt-editor'
                        elem='object'>
                        <ConfigObjectEditor
                            {...this.props}
                            title={this.props.title.substitute_title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint.substitute_hint}
                            Item={ConfigTupleEditor}
                            childProps={{
                                items: [
                                    ConfigTextEditor,
                                    ConfigTextEditor
                                ],
                                placeholder: [
                                    this.props.placeholder.replaceable_placeholder,
                                    this.props.placeholder.substitute_placeholder
                                ]
                            }}
                            value={this.state.replace}
                            onChange={this.onChangeObject} />
                    </Bem>
                }
            </Bem>
        );
    }
}

ConfigCryptEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    validation: PropTypes.array,
    path: PropTypes.string.isRequired,
    title: PropTypes.object.isRequired,
    yamlKey: PropTypes.string,
    hint: PropTypes.object,
    placeholder: PropTypes.any,
    value: PropTypes.object,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigCryptEditor;

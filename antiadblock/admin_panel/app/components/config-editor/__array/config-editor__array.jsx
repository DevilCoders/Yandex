import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';

import i18n from 'app/lib/i18n';
import isEqual from 'lodash/isEqual';

import './config-editor__array.css';

class ConfigArrayEditor extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            value: props.value
        };

        this.onAdd = this.onAdd.bind(this);
        this.onChange = this.onChange.bind(this);
        this.onRemove = this.onRemove.bind(this);
    }

    shouldComponentUpdate(nextProps, nextState) {
        return (
            !isEqual(nextProps.isDefaultValue, this.props.isDefaultValue) ||
            !isEqual(nextProps.value, this.state.value) ||
            !isEqual(nextState.value, this.state.value)
        );
    }

    componentWillReceiveProps(nextProps) {
        // Синхронизируемся только если массив пустой
        if (!this.state.value.length || !nextProps.value.length) {
            this.setState({
                value: nextProps.value
            });
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

    trim(value) {
        let to = 0;

        for (let i = value.length - 1; i >= 0; i--) {
            if (value[i]) {
                to = i + 1;
                break;
            }
        }

        return value.slice(0, to);
    }

    onAdd() {
        const {value: valueState} = this.state;
        const value = [...valueState, undefined];

        // TODO Костыль
        if (value.length === 1) {
            value.push(undefined);
        }

        this.setState({
            value
        }, () => {
            this.props.onChange(this.trim(value));
        });
    }

    getKey(value, index) {
        if (this.props.keyFunction) {
            return this.props.keyFunction(value, index);
        }

        return index;
    }

    onChange(index) {
        return (value, path) => {
            const {value: valueState} = this.state;
            const copy = valueState.slice();
            copy[index] = value;

            this.setState({
                value: copy
            });

            this.props.onChange(this.trim(copy), path);
        };
    }

    onRemove(index) {
        return () => {
            const {value: valueState} = this.state;
            const copy = valueState.slice(0, index).concat(valueState.slice(index + 1));

            this.setState({
                value: copy
            });

            this.props.onChange(this.trim(copy));
        };
    }

    render() {
        const Item = this.props.Item;
        const value = this.state.value;
        const validation = this.validate();
        const mods = {
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-array-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        block='config-array-editor'
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
                    key='list'
                    block='config-array-editor'
                    elem='list'>
                    {[...new Array(value.length || 1)].map((_, index) => {
                        const key = this.getKey(value[index], index);
                        const path = `${this.props.path}.${key}`;
                        /* eslint-disable react/no-array-index-key */
                        return (
                            <Bem
                                block='config-array-editor'
                                elem='item'
                                key={index}>
                                <Bem
                                    block='config-array-editor'
                                    elem='editor'
                                    key='editor'>
                                    <Item
                                        placeholder={this.props.placeholder}
                                        {...this.props.childProps}
                                        setEditorRef={this.props.setEditorRef}
                                        value={value[index]}
                                        path={path}
                                        offset={30}
                                        validation={this.props.validation}
                                        onChange={this.onChange(index)}
                                        onError={this.props.onError}
                                        readOnly={this.props.readOnly} />
                                </Bem>
                                {!this.props.readOnly ?
                                    <Bem
                                        block='config-array-editor'
                                        elem='close'
                                        key='close'>
                                        <Button
                                            theme='clear'
                                            onClick={this.onRemove(index)}>
                                            <Icon glyph='type-close' />
                                        </Button>
                                    </Bem> : null}
                            </Bem>
                        );
                        /* eslint-enable */
                    })}
                </Bem>
                {!this.props.readOnly ?
                    <Bem
                        block='config-array-editor'
                        elem='actions'
                        key='actions'>
                        <Button
                            view='default'
                            tone='grey'
                            type='action'
                            size='s'
                            theme='normal'
                            onClick={this.onAdd}>
                            {i18n('common', 'add')}
                        </Button>
                    </Bem> : null}
                <ConfigEditorError
                    text={validation}
                    hasSpaceTop
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigArrayEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    validation: PropTypes.array,
    Item: PropTypes.func.isRequired,
    onError: PropTypes.func,
    yamlKey: PropTypes.string,
    placeholder: PropTypes.any,
    value: PropTypes.array,
    title: PropTypes.string,
    hint: PropTypes.string,
    keyFunction: PropTypes.func,
    readOnly: PropTypes.bool,
    childProps: PropTypes.object,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

ConfigArrayEditor.defaultProps = {
    value: []
};

export default ConfigArrayEditor;

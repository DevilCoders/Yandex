import React from 'react';
import PropTypes from 'prop-types';

import ConfigArrayEditor from '../__array/config-editor__array';

import i18n from 'app/lib/i18n';

import './config-editor__object.css';

class ConfigObjectEditor extends React.PureComponent {
    constructor(props) {
        super();

        this.state = {
            value: this.buildArray(props.value)
        };

        this.onChange = this.onChange.bind(this);
        this.getKey = this.getKey.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        // Синхронизируем value только если текущее значение пусто
        if (!this.state.value.length || !Object.keys(nextProps.value).length) {
            this.setState({
                value: this.buildArray(nextProps.value)
            });
        }
    }

    validate(value) {
        let validation = [],
            keys = {};

        for (let i = 0; i < value.length; i++) {
            if (!value[i]) {
                continue;
            }

            const key = value[i][0];
            const val = value[i][1];

            // Нет ключа, но значение чем то заполнено - ошибка
            if (!key && val) {
                validation.push({
                    message: i18n('error', 'empty-key'),
                    path: [
                        ...this.props.path.split('.'),
                        i
                    ]
                });
                continue;
            }

            if (!keys[key]) {
                keys[key] = true;
                continue;
            }

            validation.push({
                message: i18n('error', 'same-key'),
                path: [
                    ...this.props.path.split('.'),
                    key
                ]
            });
        }

        return validation;
    }

    buildArray(value) {
        if (value) {
            return Object.keys(value).map(k => {
                const v = value[k];

                return [k, v];
            });
        }

        return [];
    }

    getHashTypeValueEmpty(value) {
        const hashEmpty = {
            bool: false,
            number: 0,
            string: ''
        };

        return hashEmpty[value];
    }

    buildObject(value) {
        const typeValue = this.props.config.type_schema.value;

        return value.reduce((acc, item) => {
            if (item && item[0]) {
                acc[item[0]] = item[1] || this.getHashTypeValueEmpty(typeValue);
            }

            return acc;
        }, {});
    }

    onChange(value) {
        const validation = this.validate(value);

        this.setState({
            value
        }, () => {
            // Если есть ошибки валидации, то вызываем onError
            if (validation.length) {
                this.props.onError(validation);
            } else {
                this.props.onChange(this.buildObject(value));
            }
        });
    }

    getKey(value, index) {
        if (value && value[0]) {
            return value[0];
        }

        return index;
    }

    render() {
        return (
            <ConfigArrayEditor
                {...this.props}
                value={this.state.value}
                onChange={this.onChange}
                keyFunction={this.getKey} />
        );
    }
}

ConfigObjectEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    onError: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    config: PropTypes.object.isRequired,
    title: PropTypes.string,
    hint: PropTypes.string,
    placeholder: PropTypes.any,
    value: PropTypes.object,
    validation: PropTypes.array,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigObjectEditor;

import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import TextInput from 'lego-on-react/src/components/textinput/textinput.react';
import Button from 'lego-on-react/src/components/button/button.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import {KEYS} from 'app/enums/keys';

import './tag-list.css';

const REMOVE_TIMEOUT = 100;

class TagList extends React.Component {
    constructor(props) {
        super(props);

        this._inputRef = null;
        this._removedTimeout = null;

        this.state = {
            value: '',
            selected: false
        };

        this.onKeyUp = this.onKeyUp.bind(this);
        this.onKeyDown = this.onKeyDown.bind(this);
        this.onApply = this.onApply.bind(this);
        this.onChange = this.onChange.bind(this);
        this.clearTimeout = this.clearTimeout.bind(this);
        this.getOnRemoveHandler = this.getOnRemoveHandler.bind(this);
    }

    onChange(value) {
        this.setState({
            selected: false,
            value
        });
    }

    onApply() {
        const value = this.state.value.trim();
        if (value && this.props.value.indexOf(value) === -1) {
            this.props.onChange([
                ...this.props.value,
                value
            ]);
        }

        this.setState({
            value: ''
        });
    }

    getOnRemoveHandler(removeIndex) {
        return () => {
            this.props.onChange(this.props.value.filter((_, index) => index !== removeIndex));
        };
    }

    onKeyUp(e) {
        if (e.keyCode === KEYS.ENTER) {
            this.onApply();
        }
    }

    onKeyDown(e) {
        // При нажатии backspace при пустой строке сначала отмечаем последний элемент, затем удаляем
        if (e.keyCode === KEYS.BACKSPACE && !this.state.value) {
            // Если стоит removedTimeout, то обновляем его
            if (this._removedTimeout) {
                clearTimeout(this._removedTimeout);
                this._removedTimeout = setTimeout(this.clearTimeout, REMOVE_TIMEOUT);
                return;
            }

            if (!this.state.selected) {
                this.setState({
                    selected: true
                });
            } else {
                // При удалении элемента ставим timeout, чтобы случайно нельзя было удалить весь список зажав backspace
                this._removedTimeout = setTimeout(this.clearTimeout, REMOVE_TIMEOUT);

                this.props.onChange(this.props.value.slice(0, -1));
                this.setState({
                    selected: false
                });
            }
        }
    }

    clearTimeout() {
        this._removedTimeout = null;
    }

    render() {
        return (
            <Bem
                block='tag-list'
                mods={{
                    selected: this.state.selected,
                    readOnly: this.props.readOnly
                }}>
                {this.props.value.map((value, index) => {
                    return (
                        <Bem
                            key={value}
                            block='tag-list'
                            elem='item'>
                            <Bem
                                key='value'
                                block='tag-list'
                                elem='value'>
                                {value}
                            </Bem>
                            {!this.props.readOnly &&
                                <Bem
                                    block='tag-list'
                                    elem='close'
                                    key='close'>
                                    <Button
                                        theme='clear'
                                        onClick={this.getOnRemoveHandler(index)}>
                                        <Icon type='close' />
                                    </Button>
                                </Bem>}
                        </Bem>
                    );
                })}
                <Bem
                    key='input'
                    block='tag-list'
                    elem='input'>
                    {!this.props.readOnly &&
                        <TextInput
                            controlAttrs={{
                                onKeyUp: this.onKeyUp,
                                onKeyDown: this.onKeyDown,
                                onBlur: this.onApply
                            }}
                            theme='normal'
                            tone='grey'
                            view='default'
                            size='m'
                            placeholder={this.props.readOnly ? '' : this.props.placeholder}
                            text={this.state.value}
                            onChange={this.onChange}
                            disabled={this.props.readOnly} /> }
                </Bem>
            </Bem>
        );
    }
}

TagList.propTypes = {
    value: PropTypes.array,
    onChange: PropTypes.func,
    readOnly: PropTypes.bool,
    placeholder: PropTypes.string
};

export default TagList;

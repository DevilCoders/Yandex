import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import Textarea from 'lego-on-react/src/components/textarea/textarea.react';
import Button from 'lego-on-react/src/components/button/button.react';

import './input-with-button.css';
import 'app/components/icon/_theme/icon_theme_hint.css';
import 'app/components/icon/_theme/icon_theme_attention-red.css';
import 'app/components/icon/_theme/icon_theme_copy-input.css';

import copyToClipboard from 'app/lib/copy-to-clipboard';

export default class InputWithButton extends React.Component {
    constructor() {
        super();
        this.state = {
            tooltipVisible: false
        };
        this.onButtonClick = this.onButtonClick.bind(this);
        this.onOutsideClick = this.onOutsideClick.bind(this);
        this.setButtonRef = this.setButtonRef.bind(this);
    }

    // eslint-disable-next-line no-warning-comments
    /*
        Todo придумать как объединить логику копирования с другим компнонентом (LinkCopy)
    */
    onButtonClick() {
        this.setState(state => ({
            tooltipVisible: !state.tooltipVisible
        }));

        if (this.props.timeout) {
            copyToClipboard(this.props.value);
            setTimeout(this.onOutsideClick, 500);
        }
    }

    onOutsideClick() {
        this.setState({tooltipVisible: false});
    }

    setButtonRef(button) {
        this._button = button;
    }

    render() {
        const directions = this.props.to || ['right', 'top', 'bottom'];
        const theme = this.props.theme || 'copy-input';

        return (
            <Bem
                key='input-with-button'
                block='input-with-button'>
                <Textarea
                    key='input'
                    theme='normal'
                    tone='grey'
                    view='default'
                    size='s'
                    text={this.props.value}
                    readonly={this.props.readonly}
                    pin='round-brick'
                    onChange={this.props.onChange} />
                <Button
                    theme='classic'
                    key='button'
                    view='default'
                    tone='grey'
                    size='m'
                    pin='brick-round'
                    onClick={this.onButtonClick}
                    ref={this.setButtonRef}
                    mix={{
                        block: 'input-with-button',
                        elem: 'button'
                    }}>
                    <Icon
                        key='icon'
                        mix={[{
                            block: 'icon',
                            mods: {
                                theme: theme
                            }
                        }, {
                            block: 'input-with-button',
                            elem: 'icon'
                        }]}
                        size='s' />
                </Button>
                <Tooltip
                    key='tooltip'
                    hasTail
                    mix={{
                        block: 'input-with-button',
                        elem: 'tooltip'
                    }}
                    visible={this.state.tooltipVisible}
                    anchor={this._button}
                    theme='normal'
                    view='classic'
                    size='xs'
                    autoclosable
                    onOutsideClick={this.onOutsideClick}
                    to={directions}
                    scope={this.props.scope && this.props.scope.dom} >
                    {this.props.text}
                </Tooltip>
            </Bem>
        );
    }
}

InputWithButton.propTypes = {
    value: PropTypes.string,
    theme: PropTypes.string,
    scope: PropTypes.object,
    text: PropTypes.string,
    to: PropTypes.string,
    readonly: PropTypes.bool,
    onChange: PropTypes.func,
    timeout: PropTypes.bool
};

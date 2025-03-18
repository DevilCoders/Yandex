import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import './hint.css';
import 'app/components/icon/_theme/icon_theme_hint.css';
import 'app/components/icon/_theme/icon_theme_attention-red.css';
import 'app/components/icon/_theme/icon_theme_copy-input.css';

export default class Hint extends React.Component {
    constructor() {
        super();
        this.state = {
            tooltipVisible: false
        };
        this.onIconClick = this.onIconClick.bind(this);
        this.onOutsideClick = this.onOutsideClick.bind(this);
        this.setIconRef = this.setIconRef.bind(this);
    }

    onIconClick() {
        this.setState(state => ({
            tooltipVisible: !state.tooltipVisible
        }));

        if (this.props.timeout) {
            setTimeout(this.onOutsideClick, 500);
        }
    }

    onOutsideClick() {
        this.setState({tooltipVisible: false});
    }

    setIconRef(icon) {
        this._icon = icon;
    }

    render() {
        const directions = this.props.to || ['right', 'top', 'bottom'];
        const theme = this.props.theme || 'hint';
        return (
            <Bem block='hint'>
                <Icon
                    key='icon'
                    ref={this.setIconRef}
                    attrs={{
                        onClick: this.onIconClick
                    }}
                    mix={[{
                        block: 'icon',
                        mods: {
                            theme: theme
                        }
                    }, {
                        block: 'hint',
                        elem: 'icon'
                    }]}
                    size='s' />
                <Tooltip
                    key='tooltip'
                    hasTail
                    mix={{
                        block: 'hint',
                        elem: 'tooltip'
                    }}
                    visible={this.state.tooltipVisible}
                    anchor={this._icon}
                    theme='normal'
                    view='classic'
                    size='xs'
                    autoclosable
                    onOutsideClick={this.onOutsideClick}
                    to={directions}
                    scope={this.props.scope && this.props.scope.dom}>
                    {this.props.text}
                </Tooltip>
            </Bem>
        );
    }
}

Hint.propTypes = {
    scope: PropTypes.object,
    to: PropTypes.any,
    text: PropTypes.string.isRequired,
    theme: PropTypes.string,
    timeout: PropTypes.bool
};

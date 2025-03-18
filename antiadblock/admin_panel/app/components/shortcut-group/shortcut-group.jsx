import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';

import './shortcut-group.css';
import 'app/components/icon/_theme/icon_theme_link-external.css';

class ShortcutGroup extends React.Component {
    constructor() {
        super();

        this.state = {
            groupIsVisible: false
        };

        this.onChangeVisible = this.onChangeVisible.bind(this);
    }

    onChangeVisible() {
        this.setState(state => ({
            groupIsVisible: !state.groupIsVisible
        }));
    }

    render() {
        const {
            groupIsVisible
        } = this.state;

        return (
            <Bem
                block='shortcut-group'
                mods={{
                    checked: groupIsVisible
                }}>
                <Button
                    view='classic'
                    tone='grey'
                    type='check'
                    size='m'
                    theme='normal'
                    width='max'
                    mix={{
                        block: 'shortcut-group',
                        elem: 'title-button',
                        mods: {
                            checked: groupIsVisible
                        }
                    }}
                    checked={groupIsVisible}
                    iconRight={{
                        mods: {
                            type: 'arrow',
                            direction: groupIsVisible ? 'down' : 'right'
                        }
                    }}
                    title={this.props.title}
                    onClick={this.onChangeVisible}>
                    {this.props.title}
                </Button>
                <Bem
                    block='shortcut-group'
                    elem='fields'
                    mods={{
                        visible: groupIsVisible
                    }}>
                    {groupIsVisible && this.props.children}
                </Bem>
            </Bem>
        );
    }
}

ShortcutGroup.propTypes = {
    title: PropTypes.string,
    children: PropTypes.node
};

export default ShortcutGroup;

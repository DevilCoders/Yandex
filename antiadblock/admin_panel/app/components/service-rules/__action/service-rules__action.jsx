import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';

import i18n from 'app/lib/i18n';

import Button from 'lego-on-react/src/components/button/button.react';

import './service-rules__action.css';

export default class ServiceRulesAction extends React.Component {
    constructor(props) {
        super(props);

        this.state = {...props.data};

        this.onChangeActionItem = this.onChangeActionItem.bind(this);
        this.onConfirmAction = this.onConfirmAction.bind(this);
    }

    onChangeActionItem(title, key) {
        return () => {
            this.setState(state => ({
                [title]: {
                    ...state[title],
                    [key]: !state[title][key]
                }
            }));
        };
    }

    getTranslate(title) {
        const customTitle = this.props.customTitle || {};

        return i18n('service-rules', customTitle[title] ? customTitle[title] : title);
    }

    onConfirmAction() {
        this.props.onConfirmAction(this.state);
    }

    renderActionItem(title) {
        const actions = this.state[title];

        return (
            <Bem
                block='service-rules-action'
                elem='item'
                key={`service-rules-action-item-${title}`}>
                <Bem
                    block='service-rules-action'
                    elem='item-title'>
                    {this.getTranslate(title)}:
                </Bem>
                <Bem
                    block='service-rules-action'
                    elem='item-body'>
                    {Object.entries(actions).map(([key, value]) => (
                        <Bem
                            block='service-rules-action'
                            elem='item-body-item'
                            key={`body-item-${key}`}>
                            <Checkbox
                                theme='normal'
                                size='s'
                                tone='grey'
                                view='default'
                                checked={value}
                                onChange={this.onChangeActionItem(title, key)}>
                                {this.getTranslate(key)}
                            </Checkbox>
                        </Bem>
                    ))}
                </Bem>
            </Bem>
        );
    }

    render() {
        const keys = Object.keys(this.state);

        return (
            <Bem
                block='service-rules-action'>
                {keys.map(key => this.renderActionItem(key))}
                <Bem
                    block='service-rules-action'
                    elem='confirm'>
                    <Button
                        view='default'
                        tone='grey'
                        size='xs'
                        theme='action'
                        onClick={this.onConfirmAction}>
                        {i18n('service-rules', 'confirm')}
                    </Button>
                </Bem>
            </Bem>
        );
    }
}

ServiceRulesAction.propTypes = {
    onConfirmAction: PropTypes.func,
    data: PropTypes.any,
    customTitle: PropTypes.any
};

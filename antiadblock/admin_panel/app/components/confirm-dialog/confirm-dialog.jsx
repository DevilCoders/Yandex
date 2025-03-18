import React from 'react';
import PropTypes from 'prop-types';

import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Modal from 'lego-on-react/src/components/modal/modal.react';

export default class ConfirmDialog extends React.Component {
    constructor(props) {
        super();

        this.state = {
            visible: props.visible
        };

        this.onApply = this.onApply.bind(this);
        this.onCancel = this.onCancel.bind(this);
    }

    componentWillReceiveProps(newProps) {
        this.setState({
            visible: newProps.visible
        });
    }

    onApply() {
        this.setState({
            visible: false
        });
        this.props.callback(true);
    }

    onCancel() {
        this.setState({
            visible: false
        });
        this.props.callback(false);
    }

    render() {
        return (
            <Modal visible={this.state.visible}>
                <Bem
                    key='title'
                    block='modal'
                    elem='title'>
                    {this.props.message}
                </Bem>
                <Bem
                    key='actions'
                    block='modal'
                    elem='actions'>
                    <Button
                        theme='action'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onApply}>
                        {i18n('common', 'yes')}
                    </Button>
                    <Button
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onCancel}>
                        {i18n('common', 'no')}
                    </Button>
                </Bem>
            </Modal>
        );
    }
}

ConfirmDialog.propTypes = {
    callback: PropTypes.func,
    message: PropTypes.string,
    visible: PropTypes.bool
};

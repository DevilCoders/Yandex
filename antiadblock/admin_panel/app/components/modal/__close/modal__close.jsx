import React from 'react';
import PropTypes from 'prop-types';
import Button from 'lego-on-react/src/components/button/button.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import './modal__close.css';

export default class ModalClose extends React.Component {
    render() {
        return (
            <Button
                theme='clear'
                mix={{
                    block: 'modal',
                    elem: 'close'
                }}
                onClick={this.props.onClick}>
                <Icon glyph='type-close' />
            </Button>
        );
    }
}

ModalClose.propTypes = {
    onClick: PropTypes.func
};

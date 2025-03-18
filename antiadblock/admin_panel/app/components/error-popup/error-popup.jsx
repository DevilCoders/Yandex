import React from 'react';
import {connect} from 'react-redux';
import PropTypes from 'prop-types';

import {getErrors} from 'app/reducers/index';
import {getGlobal, getDashboard} from 'app/reducers/errors';

import Bem from 'app/components/bem/bem';
import ErrorPopupWrapper from './__wrapper/error-popup__wrapper';

class ErrorPopup extends React.Component {
    constructor() {
        super();

        this.state = {
            errorPopupVisible: false
        };
        this.onClose = this.onClose.bind(this);
    }

    hasErrors(props) {
        props = props || this.props;

        return props.errors && Boolean(props.errors.length);
    }

    componentWillReceiveProps(props) {
        if (this.hasErrors(props) && props.errors !== this._prevErrors) {
            this.setState({
                errorPopupVisible: true
            });
        } else if (!this.hasErrors(props)) {
            this.setState({
                errorPopupVisible: false
            });
        }

        this._prevErrors = props.errors;
    }

    onClose() {
        this.setState({
            errorPopupVisible: false
        });
    }

    render() {
        if (this.state.errorPopupVisible) {
            return (
                <Bem
                    block='error-popup'>
                    <ErrorPopupWrapper
                        message={this.props.errors.join('\n')}
                        onClose={this.onClose} />
                </Bem>
            );
        }

        return null;
    }
}

ErrorPopup.propTypes = {
    errors: PropTypes.array
};

export default connect(state => {
    const global = getGlobal(getErrors(state));
    const dashboard = getDashboard(getErrors(state));
    return {
        errors: [...global, ...dashboard]
    };
})(ErrorPopup);

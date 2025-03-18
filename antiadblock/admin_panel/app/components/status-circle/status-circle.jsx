import React from 'react';
import PropTypes from 'prop-types';

import './status-circle.css';

export default class StatusCircle extends React.Component {
    render() {
        const size = this.props.size || 'small';
        return (
            <div
                className={`status-circle status-circle_${size} ${this.props.status ? `status-circle_${this.props.status}` : ''}`} />
        );
    }
}

StatusCircle.propTypes = {
    status: PropTypes.string.isRequired,
    size: PropTypes.oneOf(['small', 'big'])
};

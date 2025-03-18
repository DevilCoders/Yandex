import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import StatusCircle from 'app/components/status-circle/status-circle';

import './service-health__title.css';

class ServiceHealthTitle extends React.Component {
    render() {
        return (
            <Bem block='service-health-title'>
                <Bem
                    block='service-health-title'
                    elem='status'>
                    <StatusCircle
                        status={this.props.status}
                        size='big' />
                </Bem>
                <Bem
                    block='service-health-title'
                    elem='title'>
                    {this.props.title}
                </Bem>
            </Bem>
        );
    }
}

ServiceHealthTitle.propTypes = {
    status: PropTypes.string,
    title: PropTypes.string
};

export default ServiceHealthTitle;

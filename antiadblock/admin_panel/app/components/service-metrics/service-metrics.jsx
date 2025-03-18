import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ServiceMetricItem from './__item/service-metrics__item';

const template = {
    status: {
        ranges: ['15m', '30m', '1h', '4h', '12h', '24h', '4d'],
        graphs: ['status', 'percents', 'domains', 'timings']
    },
    blockers: {
        ranges: ['4h', '12h', '24h', '4d'],
        graphs: ['blockers', 'browsers', 'proportions']
    }
};

class ServiceMetrics extends React.Component {
    render() {
        return (
            <Bem block='service-metric'>
                {Object.keys(template).map(blockName => {
                    const graphs = template[blockName];
                    return (
                        <ServiceMetricItem
                            graphs={graphs.graphs}
                            ranges={graphs.ranges}
                            service={this.props.service}
                            key={blockName} />
                    );
                })}
            </Bem>
        );
    }
}

ServiceMetrics.propTypes = {
    service: PropTypes.object
};

export default ServiceMetrics;

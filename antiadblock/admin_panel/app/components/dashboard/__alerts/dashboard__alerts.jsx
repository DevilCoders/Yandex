import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import differenceWith from 'lodash/differenceWith';
import isEqual from 'lodash/isEqual';

import {getAlerts} from 'app/reducers';

import Bem from 'app/components/bem/bem';

import './dashboard__alerts.css';

import MultiTags from 'app/components/multitags/multitags';

import {prepareServiceAlerts} from 'app/lib/alerts-logic';

const ALERTS_TAGS_LIMIT = 5;

class DashboardAlerts extends React.Component {
    shouldComponentUpdate(newProps) {
        const diffAlerts = differenceWith(newProps.itemAlerts, this.props.itemAlerts, isEqual);
        const diffTailAlerts = differenceWith(newProps.itemTailAlerts, this.props.itemTailAlerts, isEqual);

        return diffAlerts.length || diffTailAlerts.length;
    }

    render() {
        return (
            <Bem
                block='dashboard'
                elem='alerts'>
                <MultiTags
                    id={this.props.id}
                    tagItems={this.props.itemAlerts}
                    otherItems={this.props.itemTailAlerts} />
            </Bem>
        );
    }
}

DashboardAlerts.propTypes = {
    id: PropTypes.string.isRequired,
    itemAlerts: PropTypes.array,
    itemTailAlerts: PropTypes.array
};

export default connect((state, props) => {
    const allAlerts = getAlerts(state).items;
    const alerts = allAlerts[props.id] || [];
    const prepareAlerts = prepareServiceAlerts(alerts, ALERTS_TAGS_LIMIT);

    return {
        itemAlerts: prepareAlerts.topAlerts ? prepareAlerts.topAlerts : [],
        itemTailAlerts: prepareAlerts.tailAlerts ? prepareAlerts.tailAlerts : []
    };
})(DashboardAlerts);

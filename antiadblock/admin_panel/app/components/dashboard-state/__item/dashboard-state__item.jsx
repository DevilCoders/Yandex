import React from 'react';
import PropTypes from 'prop-types';

import Link from 'lego-on-react/src/components/link/link.react';
import Bem from 'app/components/bem/bem';

import i18n from 'app/lib/i18n';
import {antiadbUrl} from 'app/lib/url';

import './dashboard-state__item.css';

export default class DashboardStateItem extends React.Component {
    prepareDescription() {
        const {
            isTv,
            countAlerts,
            countOutdated,
            countAllChecks
        } = this.props;

        return isTv ?
            `${countOutdated} / ${countAlerts}` :
            `${i18n('dashboard-state', 'count-all-checks')}: ${countAllChecks}
                ${i18n('dashboard-state', 'count-outdated')}: ${countOutdated}
                    ${i18n('dashboard-state', 'count-alerts')}: ${countAlerts}`;
    }

    render() {
        return (
            <Link
                mix={[{
                    block: 'dashboard-state-item',
                    mods: {
                        status: this.props.state,
                        disabled: this.props.outdated,
                        tv: this.props.isTv
                    }
                }]}
                theme='black'
                url={antiadbUrl(`/service/${this.props.id}/health`)}>
                <Bem
                    block='dashboard-state-item'
                    elem='title'>
                    {this.props.name}
                </Bem>
                <Bem
                    key='description'
                    block='dashboard-state-item'
                    elem='description'>
                    {this.prepareDescription()}
                </Bem>
            </Link>
        );
    }
}

DashboardStateItem.propTypes = {
    id: PropTypes.string.isRequired,
    state: PropTypes.string.isRequired,
    outdated: PropTypes.bool.isRequired,
    countOutdated: PropTypes.number.isRequired,
    countAllChecks: PropTypes.number.isRequired,
    countAlerts: PropTypes.number.isRequired,
    name: PropTypes.string.isRequired,
    isTv: PropTypes.bool
};

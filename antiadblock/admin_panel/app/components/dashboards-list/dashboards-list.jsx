import React from 'react';
import PropTypes from 'prop-types';

import pickBy from 'lodash/pickBy';

import Select from 'lego-on-react/src/components/select/select.react';
import Bem from 'app/components/bem/bem';

import {getPermissions} from 'app/lib/permissions';
import {getUser} from 'app/lib/user';
import i18n from 'app/lib/i18n';
import './dashboards-list.css';

export default class DashboardsList extends React.Component {
    constructor() {
        super();

        const permissions = getPermissions();
        const user = getUser();

        this.dashboards = pickBy({
            '/': {
                i18n: i18n('dashboard', 'dashboard-title')
            },
            '/heatmap': {
                permission: permissions.SERVICE_CREATE,
                i18n: i18n('dashboard', 'heatmap-title')
            }
        }, dashboard => !dashboard.permission || user.hasPermission(dashboard.permission));
    }

    render() {
        return (
            <Bem block='dashboards-list'>
                <Select
                    theme='normal'
                    view='default'
                    tone='grey'
                    size='m'
                    width='max'
                    type='radio'
                    onChange={this.props.onChange}
                    val={this.dashboards[this.props.value] && this.props.value}
                    placeholder={this.dashboards[this.props.value] || i18n('header', 'select-dashboard')} >
                    {Object.keys(this.dashboards).map(key => (
                        <Select.Item
                            key={this.dashboards[key].i18n}
                            val={key} >
                            {this.dashboards[key].i18n}
                        </Select.Item>
                    ))}
                </Select>
            </Bem>
        );
    }
}

DashboardsList.propTypes = {
    value: PropTypes.string.isRequired,
    onChange: PropTypes.func.isRequired
};

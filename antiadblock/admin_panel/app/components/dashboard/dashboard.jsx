import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {fetchAlerts} from 'app/actions/alerts';
import {getServices} from 'app/reducers/index';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import DashboardItem from './__item/dashboard__item';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';

import './dashboard.css';

import {serviceType} from 'app/types';
import i18n from 'app/lib/i18n';
import {STATUS} from 'app/enums/service';

// ten seconds
const ALERTS_REFETCH_TIME = 10 * 1000;

class Dashboard extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            showInactive: false
        };

        this.onShowInactiveChange = this.onShowInactiveChange.bind(this);

        this._timer = null;
    }

    componentDidMount() {
        this.props.fetchAlerts();
        this._timer = setInterval(() => this.props.fetchAlerts(), ALERTS_REFETCH_TIME);
    }

    componentWillUnmount() {
        clearInterval(this._timer);
    }

    onShowInactiveChange() {
        this.setState(state => ({
            showInactive: !state.showInactive
        }));
    }

    render() {
        const filter = item => (this.state.showInactive || item.status === STATUS.ACTIVE);
        const items = this.props.items.filter(filter);

        return (
            <Bem block='dashboard'>
                {!this.props.loaded ?
                    <Preloader /> :
                    [
                        <Bem
                            key='header'
                            block='dashboard'
                            elem='header'>
                            <Bem
                                block='dashboard'
                                elem='title'>
                                {i18n('dashboard', 'dashboard-title')}
                            </Bem>
                            <Bem
                                block='dashboard'
                                elem='filters'>
                                <Checkbox
                                    theme='normal'
                                    size='s'
                                    tone='grey'
                                    view='default'
                                    checked={this.state.showInactive}
                                    onChange={this.onShowInactiveChange}>
                                    {i18n('dashboard', 'service-list-show-inactive')}
                                </Checkbox>
                            </Bem>
                        </Bem>,
                        (items.length ?
                            <Bem
                                block='dashboard'
                                elem='list'
                                key='list'>
                                {items.map(item => (
                                    <DashboardItem
                                        key={item.id}
                                        id={item.id}
                                        name={item.name}
                                        status={item.status} />
                                ))}
                            </Bem> :
                            <Bem
                                block='dashboard'
                                elem='empty'
                                key='empty'>
                                {i18n('dashboard', 'not-found')}
                            </Bem>)
                    ]}
            </Bem>
        );
    }
}

Dashboard.propTypes = {
    items: PropTypes.arrayOf(serviceType).isRequired,
    loaded: PropTypes.bool.isRequired,
    fetchAlerts: PropTypes.func.isRequired
};

export default connect(state => {
    return {
        ...getServices(state)
    };
}, dispatch => {
    return {
        fetchAlerts: () => {
            dispatch(fetchAlerts());
        }
    };
})(Dashboard);

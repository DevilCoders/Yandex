import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import find from 'lodash/find';

import Bem from 'app/components/bem/bem';

import ServiceHealthItem from './__item/service-health__item';
import ServiceHealthTitle from './__title/service-health__title';

import {fetchHealth} from 'app/actions/health';

import {getService} from 'app/reducers/index';
import {getHealth} from 'app/reducers/service';

import i18n from 'app/lib/i18n';

import './service-health.css';

import {serviceType} from 'app/types';

// 1 min
const REFRESH_TIME = 60 * 1000;

class ServiceHealth extends React.Component {
    constructor(props) {
        super(props);

        this._scope = {};
        this._timer = null;

        this.setWrapperRef = this.setWrapperRef.bind(this);
    }

    componentDidMount() {
        this.props.fetchHealth(this.props.service.id);

        this._timer = setInterval(() => this.props.fetchHealth(this.props.service.id), REFRESH_TIME);
    }

    componentWillUnmount() {
        clearInterval(this._timer);
    }

    setWrapperRef(wrapper) {
        this._scope.dom = wrapper;
    }

    isValid(validTill) {
        const now = Date.now();
        const till = new Date(validTill).getTime();

        return now > till;
    }

    render() {
        const data = this.props.data;
        const groups = data.state;

        return (
            <Bem
                block='service-health'
                tagRef={this.setWrapperRef}>
                {groups.map(group => {
                    const hasErrors = Boolean(find(group.checks, item => item.state === 'red'));
                    return (
                        <Bem
                            key={group.group_id}
                            block='service-health'
                            elem='group'>
                            <ServiceHealthTitle
                                key='title'
                                status={hasErrors ? 'error' : 'ok'}
                                title={group.group_title} />
                            {group.checks && group.checks.length ?
                                <Bem
                                    key='list'
                                    block='service-health'
                                    elem='list'>
                                    {group.checks.map(item => {
                                        return (
                                            <ServiceHealthItem
                                                key={item.check_id}
                                                checkId={item.check_id}
                                                serviceId={this.props.service.id}
                                                title={item.check_title || item.check_id}
                                                description={item.value}
                                                hint={item.description}
                                                state={item.state}
                                                externalUrl={item.external_url}
                                                isValid={this.isValid(item.valid_till)}
                                                lastUpdate={item.last_update}
                                                transitionTime={item.transition_time}
                                                progressInfo={item.progress_info}
                                                inProgress={item.in_progress}
                                                scope={this._scope} />
                                        );
                                    })}
                                </Bem> :
                                <Bem
                                    key='nodata'
                                    block='service-health'
                                    elem='no-data'>
                                    {i18n('health', 'no-data')}
                                </Bem>}
                        </Bem>
                    );
                })}
            </Bem>
        );
    }
}

ServiceHealth.propTypes = {
    service: serviceType,
    data: PropTypes.object,
    fetchHealth: PropTypes.func
};

export default connect(state => {
    return {
        data: getHealth(getService(state))
    };
}, dispatch => {
    return {
        fetchHealth: serviceId => {
            return dispatch(fetchHealth(serviceId));
        }
    };
})(ServiceHealth);

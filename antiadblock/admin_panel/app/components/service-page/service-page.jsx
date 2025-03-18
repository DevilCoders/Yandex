import React from 'react';
import {connect} from 'react-redux';
import PropTypes from 'prop-types';

import {fetchService} from 'app/actions/service';
import {getService} from 'app/reducers/index';
import {Switch, Route} from 'react-router-dom';
import {fromEntries, prepareParamsToType} from 'app/lib/query-string';
import {ACTIONS, SECRET_ACTIONS} from 'app/enums/actions';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import ConfigPreview from 'app/components/config-preview/config-preview';
import ConfigApplying from 'app/components/config-applying/config-applying';
import TicketCreation from 'app/components/ticket-creation/ticket-creation';
import ExperimentApplying from 'app/components/experiment-applying/experiment-applying';
import ConfigDiffModal from 'app/components/config-diff-modal/config-diff-modal';
import ProfileDiffModal from 'app/components/profile-diff-modal/profile-diff-modal';
import ConfigDiffPage from 'app/components/config-diff-page/config-diff-page';
import ConfigDeclining from 'app/components/config-declining/config-declining';
import LabelCreate from 'app/components/label-create/label-create';
import ServicePageMenu from './__menu/service-page__menu';
import ServiceInfo from 'app/components/service-info/service-info';
import ServiceMetrics from 'app/components/service-metrics/service-metrics';
import ServiceConfigs from 'app/components/service-configs/service-configs';
import ConfigForm from 'app/components/config-form/config-form';
import ServiceSetup from 'app/components/service-setup/service-setup';
import ServiceAudit from 'app/components/service-audit/service-audit';
import ServiceUtils from 'app/components/service-utils/service-utils';
import ServiceRules from 'app/components/service-rules/service-rules';
import ServiceResultChecks from 'app/components/service-sbs/service-sbs-result-checks/service-sbs-result-checks';
import ServiceScreenshotsChecks from 'app/components/service-sbs/service-sbs-screenshots-checks/service-sbs-screenshots-checks';
import ProfileDiffPage from 'app/components/profile-diff-page/profile-diff-page';

import {serviceType} from 'app/types';

import './service-page.css';
import {getActiveLabelId} from '../../reducers/service';

class ServicePage extends React.Component {
    constructor() {
        super();

        this.state = {
            loaded: false
        };
    }

    componentDidMount() {
        this.props.fetchService(this.props.match.params.serviceId);
    }

    componentWillReceiveProps(nextProps) {
        if (nextProps.match.params.serviceId !== this.props.match.params.serviceId) {
            this.setState({
                loaded: false
            });

            this.props.fetchService(nextProps.match.params.serviceId);
        } else if (nextProps.service) {
            this.setState({
                loaded: true
            });
        }
    }

    render() {
        const active = Object.values(ACTIONS).includes(this.props.match.params.action) ? this.props.match.params.action : ACTIONS.CONFIGS;

        return (
            <Bem
                block='service-page'>
                {!this.state.loaded ?
                    <Preloader key='preloader' /> :
                    [
                            !Object.values(SECRET_ACTIONS).includes(this.props.match.params.action) ?
                                <ServicePageMenu
                                    key='menu'
                                    service={this.props.service}
                                    active={active}
                                    links={Object.values(ACTIONS)} /> :
                                null,
                        <Bem
                            key='body'
                            block='service-page'
                            elem='body'>
                            <Switch>
                                <Route path={`/service/:serviceId/${ACTIONS.INFO}`} exact>
                                    <ServiceInfo service={this.props.service} />
                                </Route>
                                <Route
                                    path={`/service/:serviceId/${ACTIONS.CONFIGS}/diff/:configId/:secondConfigId`}
                                    component={routeProps => {
                                        return (
                                            <ConfigDiffPage
                                                serviceId={this.props.service.id}
                                                id={parseInt(routeProps.match.params.configId, 10)}
                                                secondId={parseInt(routeProps.match.params.secondConfigId, 10)} />
                                        );
                                    }}
                                    exact />
                                <Route
                                    path={`/service/:serviceId/${SECRET_ACTIONS.PROFILES}/diff/:profileId/:secondProfileId`}
                                    component={routeProps => {
                                        return (
                                            <ProfileDiffPage
                                                serviceId={this.props.service.id}
                                                id={parseInt(routeProps.match.params.profileId, 10)}
                                                secondId={parseInt(routeProps.match.params.secondProfileId, 10)} />
                                        );
                                    }}
                                    exact />
                                <Route
                                    path={`/service/:serviceId/label/:labelId/${ACTIONS.CONFIGS}/:configId/status/:status?`}
                                    component={routeProps => {
                                        return (
                                            <ConfigForm
                                                status={routeProps.match.params.status}
                                                labelId={routeProps.match.params.labelId}
                                                serviceId={routeProps.match.params.serviceId}
                                                configId={routeProps.match.params.configId} />
                                        );
                                    }}
                                    exact />
                                <Route
                                    path={`/service/:serviceId/${ACTIONS.SETUP}/`}
                                    component={ServiceSetup}
                                    exact />
                                <Route path={`/service/:serviceId/${ACTIONS.AUDIT}`} exact>
                                    <ServiceAudit
                                        service={this.props.service}
                                        labelId={this.props.labelId || this.props.service.id} />
                                </Route>
                                <Route path={`/service/:serviceId/${ACTIONS.GRAPHS}`} exact>
                                    <ServiceMetrics service={this.props.service} />
                                </Route>
                                <Route path={`/service/:serviceId/${ACTIONS.UTILS}`} exact>
                                    <ServiceUtils service={this.props.service} />
                                </Route>
                                <Route path={`/service/:serviceId/${ACTIONS.RULES}`} exact>
                                    <ServiceRules service={this.props.service} />
                                </Route>
                                <Route
                                    path={`/service/:serviceId/${SECRET_ACTIONS.SCREENSHOTS}/diff/:leftRunId/:rightRunId`}
                                    render={routeProps => {
                                        const {
                                            service
                                        } = this.props;
                                        const urlParams = new URLSearchParams(location.search);
                                        const entries = urlParams.entries();
                                        const params = fromEntries(entries, true);
                                        const prepareParams = prepareParamsToType(params, {
                                            byBlockerLeft: 'string',
                                            byBlockerRight: 'string',
                                            byUrl: 'string',
                                            byBrowser: 'string',
                                            byErrorCases: 'bool'
                                        });

                                        return (
                                            <ServiceScreenshotsChecks
                                                serviceId={service.id}
                                                serviceName={service.name}
                                                routerHistory={routeProps.history}
                                                leftRunId={parseInt(routeProps.match.params.leftRunId, 10)}
                                                rightRunId={parseInt(routeProps.match.params.rightRunId, 10)}
                                                params={prepareParams} />
                                        );
                                    }}
                                    exact />
                                <Route path={`/service/:serviceId/${ACTIONS.SBS_RESULT_CHECKS}`}
                                    render={routeProps => {
                                        const urlParams = new URLSearchParams(location.search);
                                        const entries = urlParams.entries();
                                        const params = fromEntries(entries, true);
                                        const prepareParams = prepareParamsToType(params, {
                                            onlyMyRuns: 'bool',
                                            fromDate: 'string',
                                            toDate: 'string',
                                            sortedBy: 'string',
                                            isReverseSorted: 'bool',
                                            offset: 'number',
                                            limit: 'number'
                                        });

                                        return (
                                            <ServiceResultChecks
                                                service={this.props.service}
                                                routeHistory={routeProps.history}
                                                params={prepareParams} />
                                        );
                                   }} />
                                <Route path='/service/:serviceId/label/:labelId?'
                                    render={routeProps => (
                                        <ServiceConfigs
                                            service={this.props.service}
                                            labelId={routeProps.match.params.labelId} />
                                        )} />
                                <Route path='*' exact>
                                    <ServiceConfigs
                                        service={this.props.service}
                                        labelId={this.props.labelId || this.props.service.id} />
                                </Route>
                            </Switch>
                        </Bem>
                    ]}
                <ConfigPreview
                    key='preview'
                    serviceId={this.props.match.params.serviceId} />
                <ConfigApplying
                    key='applying' />
                <ConfigDiffModal
                    key='diff' />
                <ConfigDeclining />
                <ProfileDiffModal
                    key='profile-diff' />
                <LabelCreate />
                <ExperimentApplying />
                <TicketCreation />
            </Bem>
        );
    }
}

ServicePage.propTypes = {
    service: serviceType,
    fetchService: PropTypes.func.isRequired,
    match: PropTypes.object.isRequired,
    labelId: PropTypes.string
};

export default connect(state => {
    const service = getService(state);

    return {
        service: service.service,
        labelId: getActiveLabelId(service)
    };
}, dispatch => {
    return {
        fetchService: serviceId => {
            dispatch(fetchService(serviceId));
        }
    };
})(ServicePage);

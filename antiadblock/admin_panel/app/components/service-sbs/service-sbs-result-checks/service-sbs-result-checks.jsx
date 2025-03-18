import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';
import {LinkedCalendar} from 'app/components/daterangepicker';
import TableResultChecks from './__table/table-sbs-result-checks';
import Preloader from 'app/components/preloader/preloader';
import Pagination from 'app/components/pagination/pagination';
import SearchSelect from 'app/components/search-select/search-select';
import ServiceSbsProfile from 'app/components/service-sbs/service-sbs-profile/service-sbs-profile';

import {serviceType, serviceSbsResultChecksType} from 'app/types';
import {getService} from 'app/reducers';
import {getSbs, getSbsProfile, getSbsProfiles, getSbsProfilesTags, getSbsResultChecks} from 'app/reducers/service';
import {ACTIONS} from 'app/enums/actions';
import {
    fetchServiceSbsResultChecks,
    fetchServiceSbsProfileTags,
    runServiceSbsChecks,
    openServiceSbsProfile,
    fetchServiceSbsProfile
} from 'app/actions/service/sbs';
import {openConfigPreview} from 'app/actions/config-preview';
import {fetchConfigById} from 'app/actions/config';

import i18n from 'app/lib/i18n';
import {DEFAULT_TAG, getProfileTag, setProfileTag} from '../../../lib/sbs/profile-tags';
import {getQueryArgsFromObject} from 'app/lib/query-string';

import './service-sbs-result-checks.css';

// 10 seconds
const INTERVAL_TIME = 10 * 1000;
const LIMIT_ON_PAGE = 8;
const PAGE_RANGE = 5;

class ServiceSbsResultChecks extends React.Component {
    constructor(props) {
        super(props);

        this._timer = null;
        this.state = {
            selectedTag: getProfileTag(props.service && props.service.id)
        };

        this.runCheck = this.runCheck.bind(this);
        this.onChangeSortedBy = this.onChangeSortedBy.bind(this);
        this.onChangeOnlyMyRuns = this.onChangeOnlyMyRuns.bind(this);
        this.onChangeDates = this.onChangeDates.bind(this);
        this.onChangeActivePage = this.onChangeActivePage.bind(this);
        this.onOpenProfile = this.onOpenProfile.bind(this);
        this.onOpenConfig = this.onOpenConfig.bind(this);
        this.onChangeTag = this.onChangeTag.bind(this);
    }

    getDefaultState() {
        return {
            fromDate: null,
            toDate: null,
            sortedBy: 'date',
            isReverseSorted: true,
            onlyMyRuns: false,
            selectedTag: getProfileTag(this.props.service.id)
        };
    }

    getDefaultPaginationState() {
        return {
            limit: LIMIT_ON_PAGE,
            offset: 0
        };
    }

    componentWillReceiveProps(nextProps) {
        let state = {};

        if (
            !nextProps.profileTags.loading &&
            this.props.profileTags.loading &&
            !nextProps.profileTags.data.some(item => item === this.state.selectedTag)) {
            state.selectedTag = DEFAULT_TAG;
            setProfileTag(nextProps.service.id, DEFAULT_TAG);
        }

        this.setState(state);
    }

    onChangeTag(val) {
        this.setState({
            selectedTag: val
        }, () => {
            setProfileTag(this.props.service.id, val);
        });
    }

    componentDidMount() {
        this.props.fetchServiceSbsProfile(this.props.service.id);
        this.props.fetchServiceSbsProfileTags(this.props.service.id);

        this.updateSetInterval({
            ...this.getDefaultPaginationState(),
            ...this.getDefaultState(),
            ...this.props.params
        });
    }

    componentWillUnmount() {
        clearInterval(this._timer);
    }

    updateSetInterval(options) {
        const {
            fetchServiceSbsResultChecks,
            service
        } = this.props;

        if (this._timer) {
            clearInterval(this._timer);
        }

        fetchServiceSbsResultChecks(service.id, options).then(() => {
            this._timer = setInterval(() => fetchServiceSbsResultChecks(service.id, options), INTERVAL_TIME);
        });
    }

    onChangeActivePage(value) {
        const {
            params
        } = this.props;
        const limit = params.limit || this.getDefaultPaginationState().limit;
        const options = {
            ...params,
            offset: limit * (value - 1),
            limit
        };
        const queryArgs = getQueryArgsFromObject(options, true);

        this.onChangePageWithoutReload(queryArgs);
        this.updateSetInterval(options);
    }

    onChangePageWithoutReload(queryArgs) {
        const {
            routeHistory,
            service
        } = this.props;

        routeHistory.push(`/service/${service.id}/${ACTIONS.SBS_RESULT_CHECKS}${queryArgs}`);
    }

    onChangeSortedBy(value) {
        return () => {
            const {
                params
            } = this.props;
            const {
                isReverseSorted: propsIsReverseSorted,
                sortedBy
            } = params;
            const isReverseSorted = sortedBy === value ? !propsIsReverseSorted : false;
            const options = {
                ...this.getDefaultPaginationState(),
                ...params,
                isReverseSorted: isReverseSorted,
                sortedBy: value
            };
            const queryArgs = getQueryArgsFromObject(options, true);

            this.onChangePageWithoutReload(queryArgs);
            this.updateSetInterval(options);
        };
    }

    onChangeOnlyMyRuns() {
        const defaultPaginationState = this.getDefaultPaginationState();
        const {
            params
        } = this.props;
        const options = {
            ...params,
            onlyMyRuns: !params.onlyMyRuns,
            ...defaultPaginationState
        };
        const queryArgs = getQueryArgsFromObject(options, true);

        this.onChangePageWithoutReload(queryArgs);
        this.updateSetInterval(options);
    }

    onChangeDates(value) {
        const defaultPaginationState = this.getDefaultPaginationState();
        const formattedStartDate = value.startDate && value.startDate.format('YYYY-MM-DDTHH:mm:ss');
        const formattedEndDate = value.endDate && value.endDate.format('YYYY-MM-DDTHH:mm:ss');
        const options = {
            ...this.props.params,
            fromDate: formattedStartDate,
            toDate: formattedEndDate,
            ...defaultPaginationState
        };
        const queryArgs = getQueryArgsFromObject(options, true);

        this.onChangePageWithoutReload(queryArgs);
        this.updateSetInterval(options);
    }

    onOpenProfile(profileId, readOnly) {
        return () => {
            this.props.fetchServiceSbsProfileTags(this.props.service.id);
            this.props.openServiceSbsProfile(profileId, readOnly);
        };
    }

    onOpenConfig(serviceId, configId) {
        return () => {
            this.props.fetchConfigById(configId).then(res => {
                this.props.openConfigPreview(serviceId, configId, res.config);
            });
        };
    }

    runCheck() {
        const {
            runServiceSbsChecks,
            service
        } = this.props;

        runServiceSbsChecks(
            service.id,
            undefined,
            undefined,
            this.state.selectedTag !== DEFAULT_TAG ? this.state.selectedTag : undefined
        );
    }

    renderActionButtons() {
        const {
            loadingRunChecks,
            profileExist
        } = this.props;

        return (
            <Bem
                block='service-sbs-result-checks'
                elem='actions'>
                <Bem
                    block='service-sbs-result-checks'
                    elem='actions-button'>
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme={profileExist ? 'normal' : 'action'}
                        mix={{
                            block: 'service-sbs-result-checks',
                            elem: 'change-profile-button'
                        }}
                        onClick={this.onOpenProfile()}>
                        {i18n('sbs', profileExist ? 'change-profile' : 'create-profile')}
                    </Button>
                    <Button
                        view='default'
                        tone='grey'
                        type='action'
                        size='s'
                        theme='action'
                        mix={{
                            block: 'service-sbs-result-checks',
                            elem: 'run-test-button'
                        }}
                        disabled={!profileExist || loadingRunChecks}
                        onClick={this.runCheck}>
                        {i18n('sbs', 'run-test')}
                    </Button>
                </Bem>
                <Bem
                    block='service-sbs-result-checks'
                    elem='actions-profile-tags'>
                    <Bem>
                        {i18n('sbs', 'run-for-tag')}
                    </Bem>
                    <Bem
                        block='service-sbs-result-checks'
                        elem='select-tags'>
                        <SearchSelect
                            selectItems={this.props.profileTags.data}
                            selectedItemText={this.state.selectedTag}
                            onChangeSelect={this.onChangeTag} />
                    </Bem>
                </Bem>
                {this.renderFilters()}
            </Bem>
        );
    }

    renderFilters() {
        const {
            params
        } = this.props;
        const {
            onlyMyRuns
        } = params;

        return (
            <Bem
                key='filters'
                block='service-sbs-result-checks'
                elem='filters'>
                <Bem
                    block='service-sbs-result-checks'
                    elem='filters-only-run'>
                    <Checkbox
                        mix={{
                            block: 'service-sbs-result-checks',
                            elem: 'checkbox'
                        }}
                        theme='normal'
                        size='s'
                        tone='grey'
                        view='default'
                        checked={onlyMyRuns}
                        onChange={this.onChangeOnlyMyRuns}>
                        {i18n('sbs', 'only-my-run')}
                    </Checkbox>
                </Bem>
                <LinkedCalendar
                    autoApply
                    showDropdowns
                    onDatesChange={this.onChangeDates} />
            </Bem>
        );
    }

    renderWarning(key) {
        return (
            <Bem
                key='sbs-empty-table'
                block='service-sbs-result-checks'
                elem='empty'>
                {i18n('sbs', key)}
            </Bem>
        );
    }

    render() {
        const {
            service,
            resultChecks,
            sbsProfileLoaded,
            profileExist,
            loadingProfile
        } = this.props;
        const {
            loaded,
            schema,
            data
        } = resultChecks;
        const state = {
            ...this.getDefaultState(),
            ...this.getDefaultPaginationState(),
            ...this.props.params
        };
        const {
            sortedBy,
            isReverseSorted,
            limit,
            offset
        } = state;
        const activePage = Math.ceil((data.total - (data.total - offset)) / limit) + 1;

        return (
            <Bem
                block='service-sbs-result-checks'>
                {!loaded && <Preloader />}
                {(profileExist || !loadingProfile) ? this.renderActionButtons() : <Preloader />}
                {(!profileExist && !loadingProfile && sbsProfileLoaded) ? this.renderWarning('empty-profile') : null}
                {(profileExist && loaded && [
                    data.total ?
                        <TableResultChecks
                            key='sbs-table'
                            serviceId={service.id}
                            onChangeSortedBy={this.onChangeSortedBy}
                            onOpenConfig={this.onOpenConfig}
                            sortedBy={sortedBy}
                            schema={schema}
                            data={data}
                            onOpenProfile={this.onOpenProfile}
                            isReverseSorted={isReverseSorted} /> : this.renderWarning('empty-checks'),
                    data.total > limit ?
                        <Pagination
                            key='sbs-pagination'
                            activePage={activePage}
                            itemsCountPerPage={limit}
                            totalItemsCount={data.total}
                            pageRangeDisplayed={PAGE_RANGE}
                            onChange={this.onChangeActivePage} /> : null
                ])}
                <ServiceSbsProfile
                    serviceId={service.id}
                    selectedTag={this.state.selectedTag} />
            </Bem>
        );
    }
}

ServiceSbsResultChecks.propTypes = {
    service: serviceType.isRequired,
    fetchServiceSbsResultChecks: PropTypes.func,
    runServiceSbsChecks: PropTypes.func,
    resultChecks: serviceSbsResultChecksType,
    loadingRunChecks: PropTypes.bool,
    routeHistory: PropTypes.object,
    params: PropTypes.object,
    openServiceSbsProfile: PropTypes.func.isRequired,
    fetchServiceSbsProfile: PropTypes.func.isRequired,
    fetchServiceSbsProfileTags: PropTypes.func.isRequired,
    fetchConfigById: PropTypes.func.isRequired,
    openConfigPreview: PropTypes.func.isRequired,
    sbsProfileLoaded: PropTypes.bool.isRequired,
    profileExist: PropTypes.bool.isRequired,
    loadingProfile: PropTypes.bool.isRequired,
    profileTags: PropTypes.any

};

export default connect(state => {
    const service = getService(state);
    const sbs = getSbs(service);
    const sbsProfiles = getSbsProfiles(service);

    return {
        loadingRunChecks: sbs.loadingRunChecks,
        profileExist: sbsProfiles.exist,
        loadingProfile: sbsProfiles.loading,
        resultChecks: getSbsResultChecks(service),
        sbsProfileLoaded: getSbsProfile(service).loaded,
        profileTags: getSbsProfilesTags(service)
    };
}, dispatch => {
    return {
        fetchServiceSbsResultChecks: (serviceId, options) => {
            return dispatch(fetchServiceSbsResultChecks(serviceId, options));
        },
        fetchConfigById: configId => {
            return dispatch(fetchConfigById(configId));
        },
        fetchServiceSbsProfile: serviceId => {
            return dispatch(fetchServiceSbsProfile(serviceId));
        },
        runServiceSbsChecks: (serviceId, testing, expId, tag) => {
            return dispatch(runServiceSbsChecks(serviceId, testing, expId, tag));
        },
        openServiceSbsProfile: (profileId, readOnly) => {
            dispatch(openServiceSbsProfile(profileId, readOnly));
        },
        openConfigPreview: (serviceId, configId, configData) => {
            dispatch(openConfigPreview(serviceId, configId, configData));
        },
        fetchServiceSbsProfileTags: serviceId => (
            dispatch(fetchServiceSbsProfileTags(serviceId))
        )
    };
})(ServiceSbsResultChecks);


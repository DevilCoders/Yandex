import React from 'react';
import {connect} from 'react-redux';
import PropTypes from 'prop-types';

import {fetchServiceSbsCheckScreenshots, fetchServiceSbsResultChecks} from 'app/actions/service/sbs';
import {
    getSbsScreenshotsChecksDataByRunId,
    getSbsResultChecks,
    getSbsScreenshotsChecks
} from 'app/reducers/service';
import {getService} from 'app/reducers';
import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import ServiceSbsScreenshotsChecksAction from './__action/service-sbs-screenshots-checks__action';
import ServiceSbsScreenshotsChecksList from './__list/service-sbs-screenshots-checks__list';
import ServiceSbsScreenshotsChecksModal from './__modal/service-sbs-screenshots-checks__modal';

import {SECRET_ACTIONS} from 'app/enums/actions';
import {sbsRunIdDataType} from 'app/types';
import {RUN_ID_STATUS, DEFAULT_FILTER_VALUE, DEFAULT_BLOCKER_VALUE} from 'app/enums/sbs';

import './service-sbs-screenshots-checks.css';

const defaultStateFilters = {
    byBrowser: DEFAULT_FILTER_VALUE,
    byUrl: DEFAULT_FILTER_VALUE,
    byBlockerLeft: DEFAULT_BLOCKER_VALUE,
    byBlockerRight: DEFAULT_FILTER_VALUE,
    byErrorCases: false
};

class ServiceSbsScreenshotsChecks extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            filters: {
                byBrowser: props.params.byBrowser || DEFAULT_FILTER_VALUE,
                byUrl: props.params.byUrl || DEFAULT_FILTER_VALUE,
                byBlockerLeft: props.params.byBlockerLeft || DEFAULT_BLOCKER_VALUE,
                byBlockerRight: props.params.byBlockerRight || DEFAULT_FILTER_VALUE,
                byErrorCases: props.params.byErrorCases || false
            },
            modalVisible: false,
            modalData: {}
        };

        this.onChangeFilter = this.onChangeFilter.bind(this);
        this.onResetFilters = this.onResetFilters.bind(this);
        this.onChangeModalVisible = this.onChangeModalVisible.bind(this);
        this.setRoute = this.setRoute.bind(this);
    }

    componentDidMount() {
        const {
            serviceId,
            leftRunId,
            rightRunId,
            fetchServiceSbsCheckScreenshots,
            fetchServiceSbsResultChecks
        } = this.props;

        fetchServiceSbsCheckScreenshots(serviceId, leftRunId);

        if (leftRunId !== rightRunId) {
            fetchServiceSbsCheckScreenshots(serviceId, rightRunId);
        }

        fetchServiceSbsResultChecks(serviceId);
    }

    onChangeModalVisible(data) {
        this.setState(state => ({
            modalVisible: !state.modalVisible,
            modalData: state.modalVisible ? {} : data
        }));
    }

    onResetFilters() {
        this.setRoute(this.props.leftRunId, this.props.rightRunId, true);
        this.setState({
            filters: {
              byBrowser: DEFAULT_FILTER_VALUE,
              byUrl: DEFAULT_FILTER_VALUE,
              byBlockerLeft: DEFAULT_BLOCKER_VALUE,
              byBlockerRight: DEFAULT_FILTER_VALUE,
              byErrorCases: false
            }
        });
    }

    onChangeFilter(key, val) {
        if (
            (key === 'leftRunId' && val !== this.props.leftRunId) ||
            (key === 'rightRunId' && val !== this.props.rightRunId)
        ) {
            fetchServiceSbsCheckScreenshots(this.props.serviceId, val);

            const ids = {
                rightRunId: this.props.rightRunId,
                leftRunId: this.props.leftRunId,
                [key]: val
            };

            this.setRoute(ids.leftRunId, ids.rightRunId, true);
        } else {
            this.setState(state => ({
                filters: {
                    ...state.filters,
                    [key]: val
                }
            }));
            setTimeout(() => (this.setRoute(this.props.leftRunId, this.props.rightRunId)));
        }
    }

    setRoute(leftRunId, rightRunId, isDefault) {
        const filters = isDefault ? defaultStateFilters : this.state.filters;
        const query = encodeURI('?byBrowser=' + filters.byBrowser + '&byBlockerLeft=' +
            filters.byBlockerLeft + '&byBlockerRight=' + filters.byBlockerRight + '&byErrorCases=' + filters.byErrorCases) +
            '&byUrl=' + encodeURIComponent(filters.byUrl);

        this.props.routerHistory.push(
            `/service/${this.props.serviceId}/${SECRET_ACTIONS.SCREENSHOTS}/diff/${leftRunId}/${rightRunId}${query}`
        );
    }

    isValidData() {
        const {
            dataLeft,
            dataRight
        } = this.props;

        return dataLeft.cases && dataRight.cases && dataLeft.cases.length && dataRight.cases.length;
    }

    getActionEnums(casesLeft, casesRight) {
        const enumUrls = new Set([DEFAULT_FILTER_VALUE]);
        const enumBrowsers = new Set([DEFAULT_FILTER_VALUE]);
        const enumBlockers = new Set([DEFAULT_FILTER_VALUE, DEFAULT_BLOCKER_VALUE]);

        [casesLeft, casesRight].forEach(cases => (
           cases.forEach(item => {
               enumUrls.add(item.url);
               enumBrowsers.add(item.browser);
               enumBlockers.add(item.adblocker);
           })
        ));

        return {
            urls: Array.from(enumUrls),
            browsers: Array.from(enumBrowsers),
            blockers: Array.from(enumBlockers)
        };
    }

    filterCases(cases, option) {
        const keys = Object.keys(option);

        return cases.filter(item => (
            keys.every(key => (
                option[key] === item[key] || option[key] === 'all'
            ))
        ));
    }

    render() {
        const {
            dataLeft,
            dataRight,
            enumRunId,
            leftRunId,
            rightRunId,
            loadedResultChecks,
            loadedCheckScreenshots
        } = this.props;

        if (!(loadedCheckScreenshots && loadedResultChecks)) {
            return <Preloader />;
        }

        const filteringCasesLeft = this.filterCases(dataLeft.cases, {
            adblocker: this.state.filters.byBlockerLeft,
            browser: this.state.filters.byBrowser,
            url: this.state.filters.byUrl
        });
        const filteringCasesRight = this.filterCases(dataRight.cases, {
            adblocker: this.state.filters.byBlockerRight,
            browser: this.state.filters.byBrowser,
            url: this.state.filters.byUrl
        });

        return (
            <Bem
                block='service-sbs-screenshots-checks'>
                <Bem
                    block='service-sbs-screenshots-checks'
                    elem='header'>
                    <Bem
                        block='service-sbs-screenshots-checks'
                        elem='header-action'>
                        {this.isValidData() ?
                            <ServiceSbsScreenshotsChecksAction
                                filters={{
                                    ...this.state.filters,
                                    byRunIdLeft: leftRunId,
                                    byRunIdRight: rightRunId
                                }}
                                enums={{
                                    ...this.getActionEnums(dataLeft.cases, dataRight.cases),
                                    runIds: enumRunId
                                }}
                                onResetFilters={this.onResetFilters}
                                onChangeFilter={this.onChangeFilter} /> : null}
                    </Bem>
                </Bem>
                <Bem
                    block='service-sbs-screenshots-checks'
                    elem='body'>
                    {this.isValidData() ?
                        <ServiceSbsScreenshotsChecksList
                            casesLeft={filteringCasesLeft || []}
                            casesRight={filteringCasesRight || []}
                            leftRunId={leftRunId}
                            rightRunId={rightRunId}
                            onlyErrorCases={this.state.filters.byErrorCases}
                            onChangeFilter={this.onChangeFilter}
                            onChangeModalVisible={this.onChangeModalVisible} /> :
                        <Bem
                            block='service-sbs-screenshots-checks'
                            elem='warning'>
                            {i18n('sbs', 'status-warning')}
                        </Bem>}
                </Bem>
                <ServiceSbsScreenshotsChecksModal
                    data={this.state.modalData}
                    onChangeVisible={this.onChangeModalVisible}
                    visible={this.state.modalVisible} />
            </Bem>
        );
    }
}

ServiceSbsScreenshotsChecks.propTypes = {
    serviceId: PropTypes.string.isRequired,
    leftRunId: PropTypes.number.isRequired,
    rightRunId: PropTypes.number.isRequired,
    dataLeft: sbsRunIdDataType,
    dataRight: sbsRunIdDataType,
    fetchServiceSbsCheckScreenshots: PropTypes.func.isRequired,
    fetchServiceSbsResultChecks: PropTypes.func.isRequired,
    enumRunId: PropTypes.array.isRequired,
    routerHistory: PropTypes.object,
    loadedResultChecks: PropTypes.bool,
    loadedCheckScreenshots: PropTypes.bool,
    params: PropTypes.any
};

export default connect((state, props) => {
    const service = getService(state);
    const resultChecks = getSbsResultChecks(service);
    const screenshotsChecks = getSbsScreenshotsChecks(service);
    const dataLeft = getSbsScreenshotsChecksDataByRunId(service, props.leftRunId);
    const dataRight = getSbsScreenshotsChecksDataByRunId(service, props.rightRunId);
    let enumRunId = [];

    if (resultChecks.loaded) {
        enumRunId = resultChecks.data.items.reduce((arr, item) => {
            if (item.status === RUN_ID_STATUS.SUCCESS) {
                arr.push(item.id);
            }

            return arr;
        }, []);
    }

    return {
        dataLeft,
        dataRight,
        loadedResultChecks: resultChecks.loaded,
        loadedCheckScreenshots: screenshotsChecks.loaded && dataLeft.loaded && dataRight.loaded,
        enumRunId
    };
}, dispatch => {
    return {
        fetchServiceSbsCheckScreenshots: (serviceId, runId) => {
            dispatch(fetchServiceSbsCheckScreenshots(serviceId, runId));
        },
        fetchServiceSbsResultChecks: serviceId => {
            dispatch(fetchServiceSbsResultChecks(serviceId));
        }
    };
})(ServiceSbsScreenshotsChecks);

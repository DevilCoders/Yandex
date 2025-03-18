import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {fetchAlerts} from 'app/actions/alerts';
import {getAlerts, getServices} from 'app/reducers';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import DashboardStateItem from './__item/dashboard-state__item';
import Select from 'lego-on-react/src/components/select/select.react';
import Pagination from 'app/components/pagination/pagination';

import './dashboard-state.css';

import {serviceType} from 'app/types';
import i18n from 'app/lib/i18n';
import {STATUS} from 'app/enums/service';

import {prepareStates} from 'app/lib/states-logic';

const PAGE_RANGE = 5;
const LIST_COUNT_ITEMS_ON_PAGE = [5, 10, 20, 30];

// 10 seconds
const INTERVAL_TIME = 10 * 1000;

class DashboardState extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            activePage: 1,
            // Большое число, чтобы в самом начале выводились все сервисы
            countItemsOnPage: 500
        };

        this.onChangeActivePage = this.onChangeActivePage.bind(this);
        this.onChangeCountItemsOnPage = this.onChangeCountItemsOnPage.bind(this);

        this._timer = null;
    }

    componentDidMount() {
        this.props.fetchAlerts();
        this._timer = setInterval(() => this.props.fetchAlerts(), INTERVAL_TIME);
    }

    componentWillUnmount() {
        clearInterval(this._timer);
    }

    onChangeActivePage(value) {
        this.setState({
           activePage: value
        });
    }

    onChangeCountItemsOnPage(value) {
        if (value[0] !== this.state.countItemsOnPage) {
            this.setState({
                countItemsOnPage: value[0],
                activePage: 1
            });
        }
    }

    getItemsWithChecks(items, states) {
        return items.filter(item => states[item.id] && Boolean(states[item.id].countAllChecks));
    }

    getCountItemsWithChecks(countItemsWithChecks) {
        return Object.keys(countItemsWithChecks).length;
    }

    getDashboardItemOnInterval(items, first, last, countItems) {
        let start = first * countItems,
            end = (items.length > last * countItems ? last * countItems : items.length);

        return items.slice(start, end);
    }

    render() {
        const filter = item => (item.status === STATUS.ACTIVE);

        let items,
            countItemsWithChecks;

        if (this.props.dataLoaded && this.props.loaded) {
            const itemsWithChecks = this.getItemsWithChecks(this.props.items, this.props.states).filter(filter);
            countItemsWithChecks = this.getCountItemsWithChecks(itemsWithChecks);
            items = this.getDashboardItemOnInterval(itemsWithChecks, this.state.activePage - 1, this.state.activePage, this.state.countItemsOnPage);
        }

        return (
            <Bem block='dashboard-state'>
                {!this.props.loaded || !this.props.dataLoaded ?
                    <Preloader /> : [
                        <Bem
                            key='header'
                            block='dashboard-state'
                            elem='header'>
                            <Bem
                                block='dashboard-state'
                                elem='title'>
                                {i18n('dashboard-state', 'dashboard-state-title')}
                            </Bem>
                            {(countItemsWithChecks ?
                                <Bem
                                    block='dashboard-state'
                                    elem='count-list'>
                                    <Select
                                        theme='normal'
                                        size='m'
                                        type='radio'
                                        placeholder={this.state.countItemsOnPage < countItemsWithChecks ? this.state.countItemsOnPage : i18n('dashboard-state', 'select-all')}
                                        val={this.state.countItemsOnPage < countItemsWithChecks ? this.state.countItemsOnPage : countItemsWithChecks}
                                        onChange={this.onChangeCountItemsOnPage}>
                                        {LIST_COUNT_ITEMS_ON_PAGE.map(countItem => (
                                            countItemsWithChecks > countItem ?
                                                <Select.Item
                                                    key={`count-on-page${countItem}`}
                                                    val={countItem}>
                                                    {countItem}
                                                </Select.Item> : null
                                        ))}
                                        <Select.Item
                                            val={countItemsWithChecks}>
                                            {i18n('dashboard-state', 'select-all')}
                                        </Select.Item>
                                    </Select>
                                </Bem> : null)}
                        </Bem>,
                        (items.length ?
                            <Bem
                                block='dashboard-state'
                                elem='list'
                                key='list'
                                mods={{
                                    tv: this.props.isTv
                                }}>
                                {items.map(item => (
                                    this.props.states[item.id].countAllChecks ?
                                        <DashboardStateItem
                                            key={item.id}
                                            id={item.id}
                                            name={item.name}
                                            isTv={this.props.isTv}
                                            state={this.props.states[item.id].state}
                                            outdated={this.props.states[item.id].countOutdated !== 0}
                                            countAlerts={this.props.states[item.id].countAlerts}
                                            status={item.status}
                                            countAllChecks={this.props.states[item.id].countAllChecks}
                                            countOutdated={this.props.states[item.id].countOutdated} /> : null
                                ))}
                            </Bem> :
                            <Bem
                                block='dashboard-state'
                                elem='empty'
                                key='empty'>
                                {i18n('dashboard-state', 'not-found')}
                            </Bem>),
                        (this.props.dataLoaded && (this.state.countItemsOnPage < countItemsWithChecks) ?
                            <Pagination
                                key='pagination'
                                activePage={this.state.activePage}
                                itemsCountPerPage={this.state.countItemsOnPage}
                                totalItemsCount={countItemsWithChecks}
                                pageRangeDisplayed={PAGE_RANGE}
                                onChange={this.onChangeActivePage} /> : null)
                    ]}
            </Bem>
        );
    }
}

DashboardState.propTypes = {
    items: PropTypes.arrayOf(serviceType).isRequired,
    dataLoaded: PropTypes.bool.isRequired,
    loaded: PropTypes.bool.isRequired,
    states: PropTypes.shape(PropTypes.shape({
        countAlerts: PropTypes.number,
        countAllChecks: PropTypes.number,
        countOutdated: PropTypes.number,
        state: PropTypes.string
    })),
    isTv: PropTypes.bool,
    fetchAlerts: PropTypes.func.isRequired
};

export default connect(state => {
    const servicesProps = getServices(state);
    const allAlerts = getAlerts(state);
    const states = prepareStates(allAlerts.items);

    return {
        items: servicesProps.items,
        dataLoaded: allAlerts.loaded,
        loaded: servicesProps.loaded,
        states: states
    };
}, dispatch => {
    return {
        fetchAlerts: () => {
            dispatch(fetchAlerts());
        }
    };
})(DashboardState);

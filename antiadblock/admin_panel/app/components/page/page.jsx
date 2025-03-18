import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import {Switch, Route} from 'react-router-dom';

import i18n from 'app/lib/i18n';

import Header from 'app/components/header/header';

import Bem from 'app/components/bem/bem';
import ServicePage from 'app/components/service-page/service-page';
import SupportPage from 'app/components/support-page/support-page';
import ServiceForm from 'app/components/service-form/service-form';
import ConfirmDialog from 'app/components/confirm-dialog/confirm-dialog';
import ErrorPopup from 'app/components/error-popup/error-popup';
import Dashboard from 'app/components/dashboard/dashboard';
import DashboardState from 'app/components/dashboard-state/dashboard-state';
import ServiceTableLogs from 'app/components/service-sbs/sbs-table-logs/sbs-table-logs';
import Heatmap from 'app/components/heatmap/heatmap';
import SearchPage from 'app/components/search/search';

import Footer from 'lego-on-react/src/components/footer/footer.react';
import Copyright from 'lego-on-react/src/components/copyright/copyright.react';

import './page.css';

class Page extends React.Component {
    constructor(props) {
        super(props);

        this._scope = {};

        this.setWrapperRef = this.setWrapperRef.bind(this);
    }

    setWrapperRef(wrapper) {
        this._scope.dom = wrapper;
    }

    getChildContext() {
        return {
            scope: this._scope
        };
    }

    render() {
        return (
            <Bem
                block='page'
                tagRef={this.setWrapperRef}>
                <ErrorPopup />
                <ConfirmDialog
                    visible={this.props.confirmDialog.visible}
                    message={this.props.confirmDialog.message}
                    callback={this.props.confirmDialog.callback} />
                <Header
                    serviceId={this.props.match.params.serviceId}
                    pattern={this.props.location.search} />
                <Bem
                    key='body'
                    block='page'
                    elem='body'>
                    <Switch>
                        <Route path='/service' exact component={ServiceForm} />
                        <Route path='/support' exact component={SupportPage} />
                        <Route path='/search' exact component={SearchPage} />
                        <Route path='/service/:serviceId/:action?' component={ServicePage} />
                        <Route path='/service/:serviceId/label/:labelId?' component={ServicePage} />
                        <Route path='/heatmap' exact component={Heatmap} />
                        <Route
                            path='/state/:deviceType?'
                            render={routeProps => {
                                return (
                                    <DashboardState
                                        isTv={String(routeProps.match.params.deviceType).toLowerCase() === 'tv'} />
                                );
                            }}
                            exact />
                        <Route
                            path='/logs/view/res_id/:resId/type/:type/req_id/:reqId'
                            render={routeProps => {
                                return (
                                    <ServiceTableLogs
                                        resId={routeProps.match.params.resId}
                                        type={routeProps.match.params.type}
                                        reqId={routeProps.match.params.reqId} />
                                );
                            }}
                            exact />
                        <Route path='*'>
                            <Dashboard />
                        </Route>
                    </Switch>
                </Bem>
                <Bem
                    key='footer'
                    block='page'
                    elem='footer'>
                    <Footer
                        tld='ru'
                        region='ru' >
                        <Footer.Column key='column1'>
                            <Footer.Link url='https://yandex.ru/company/'>{i18n('common', 'about')}</Footer.Link>
                            <Footer.Link url='/support'>{i18n('common', 'support')}</Footer.Link>
                        </Footer.Column>
                        <Footer.Column key='column2' side='right'>
                            <Copyright>
                                <Footer.Link url='https://yandex.ru'>{i18n('common', 'yandex')}</Footer.Link>
                            </Copyright>
                        </Footer.Column>
                    </Footer>
                </Bem>
            </Bem>
        );
    }
}

Page.propTypes = {
    match: PropTypes.object,
    location: PropTypes.object,
    confirmDialog: PropTypes.object
};

Page.childContextTypes = {
    scope: PropTypes.object
};

Page.contextTypes = {
    lang: PropTypes.string
};

export default connect(state => {
    return {
        confirmDialog: state.confirmDialog
    };
})(Page);

import React from 'react';
import PropTypes from 'prop-types';
import {Link} from 'react-router-dom';
import {connect} from 'react-redux';

import {antiadbUrl} from 'app/lib/url';
import {getUser} from 'app/lib/user';
import {getPermissions} from 'app/lib/permissions';
import {setUserUsername} from 'app/actions/user';

import ServicesList from 'app/components/services-list/services-list';
import LegoHeader from 'lego-on-react/src/components/header/header.react';
import User from 'lego-on-react/src/components/user/user.react';
import UserAccount from 'lego-on-react/src/components/user-account/user-account.react';
import DashboardsList from 'app/components/dashboards-list/dashboards-list';
import Logo from 'lego-on-react/src/components/logo/logo.react';
import LegoLink from 'lego-on-react/src/components/link/link.react';

import Bem from 'app/components/bem/bem';
import SearchInput from 'app/components/search-input/search-input';

import i18n from 'app/lib/i18n';

import './header.css';

const yaHost = location.hostname.split('.').slice(-2).join('.'); // TODO: не забыть сделать лучше. Для com.tr, например, сломается

class Header extends React.Component {
    constructor(props) {
        super(props);

        const user = getUser();

        this.state = {
            user: null,
            userIsLoggedIn: Boolean(user.user_id)
        };

        this.onChangeHref = this.onChangeHref.bind(this);
    }

    onChangeHref(pathName) {
        location = antiadbUrl(pathName);
    }

    componentDidMount () {
        const user = getUser();
        const yandexUid = user.yandexUid;

        if (!this.state.userIsLoggedIn) {
            return;
        }
        const cb = 'onPassportLoad';
        const script = document.createElement('script');

        window.onPassportLoad = user => {
            if (!user.accounts) {
                this.setState({
                    userIsLoggedIn: false
                });

                return;
            }
            const defaultAccount = user.accounts.find(account => account.uid === user.default_uid);
            const accounts = user.accounts.filter(account => account.uid !== user.default_uid);

            this.props.setUserUsername(defaultAccount.login);

            this.setState({
                user: {
                    ...user,
                    ...defaultAccount,
                    yandexUid,
                    accounts
                }
            });
        };

        script.src = `https://pass.${yaHost}/accounts?callback=${cb}&yu=${yandexUid}`;
        script.async = true;

        document.body.appendChild(script);
    }

    render() {
        const user = getUser();
        const permissions = getPermissions();

        return (
            <Bem
                block='header'>
                <LegoHeader
                    key='header'>
                    <LegoHeader.Main>
                        <LegoHeader.Logo
                            size='m'>
                            <LegoLink
                                theme='link'
                                url={`https://yandex.${this.context.lang}`}>
                                <Logo
                                    name={`ys-${this.context.lang}-86x35`}
                                    size='m' />
                            </LegoLink>
                            <Bem
                                block='b-page'
                                elem='service-logo'>
                                <Link
                                    to={antiadbUrl('/')}>
                                    <img src={`https://yastatic.net/q/logoaas/v1/${i18n('common', 'service-name')}.svg`} />
                                </Link>
                            </Bem>
                        </LegoHeader.Logo>
                        {this.state.userIsLoggedIn && user.hasPermission(permissions.INTERFACE_SEE) && (
                            <LegoHeader.Left>
                                <Bem
                                    block='header'
                                    elem='left'>
                                    <Bem
                                        block='header'
                                        elem='service-list'>
                                        <ServicesList
                                            onChange={this.onChangeHref}
                                            serviceId={this.props.serviceId} />
                                    </Bem>
                                    <Bem
                                        block='header'
                                        elem='config-search'>
                                        <SearchInput
                                            pattern={this.props.pattern} />
                                    </Bem>
                                    <Bem
                                        block='header'
                                        elem='dashboard-list'>
                                        <DashboardsList
                                            value={location.pathname}
                                            onChange={this.onChangeHref} />
                                    </Bem>
                                </Bem>
                            </LegoHeader.Left>
                        )}
                        <LegoHeader.Right>
                            {!this.state.userIsLoggedIn && <User />}
                            {this.state.user && this.state.userIsLoggedIn && (
                                <User
                                    uid={this.state.user.uid}
                                    yu={this.state.user.yandexUid}
                                    retpath={location.href}
                                    passportHost={`https://passport.${yaHost}`}
                                    name={this.state.user.displayName.name}
                                    avatarId={this.state.user.displayName.default_avatar}>
                                    {this.state.user.accounts.map(account => (
                                        <UserAccount
                                            uid={account.uid}
                                            hasTicker={account.hasTicker}
                                            tickerCount={account.tickerCount}
                                            key={`user-account-${account.uid}`}
                                            pic
                                            avatarId={account.displayName.default_avatar}
                                            name={account.displayName.name}
                                            subname={account.defaultEmail || account.displayName.social}
                                            hasRedLetter
                                            mix={{block: 'user2', elem: 'account', js: {uid: account.uid}}} />
                                    ))}
                                </User>
                            )}
                        </LegoHeader.Right>
                    </LegoHeader.Main>
                </LegoHeader>
            </Bem>
        );
    }
}

Header.propTypes = {
    serviceId: PropTypes.string,
    pattern: PropTypes.string,
    setUserUsername: PropTypes.func
};
Header.contextTypes = {
    lang: PropTypes.string
};

export default connect(() => {
    return {};
}, dispatch => {
    return {
        setUserUsername: value => {
            dispatch(setUserUsername(value));
        }
    };
})(Header);

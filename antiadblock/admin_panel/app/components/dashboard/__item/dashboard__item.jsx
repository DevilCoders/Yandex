import React from 'react';
import PropTypes from 'prop-types';

import Button from 'lego-on-react/src/components/button/button.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import Link from 'lego-on-react/src/components/link/link.react';

import Bem from 'app/components/bem/bem';

import './dashboard__item.css';
import {antiadbUrl} from 'app/lib/url';
import StatusCircle from 'app/components/status-circle/status-circle';
import i18n from 'app/lib/i18n';
import {ACTIONS} from 'app/enums/actions';
import {STATUS} from 'app/enums/service';

export default class DashboardItem extends React.Component {
    render() {
        return (
            <Bem
                block='dashboard'
                elem='item'>
                <Bem
                    block='dashboard'
                    elem='item-name'>
                    <Link
                        theme='black'
                        url={antiadbUrl(`/service/${this.props.id}`)}>
                        <Icon
                            mix={{
                                block: 'dashboard',
                                elem: 'item-icon'
                            }}>
                            <StatusCircle status={this.props.status} />
                        </Icon>
                        {this.props.name}
                    </Link>
                </Bem>
                <Bem
                    block='dashboard'
                    elem='item-actions'>
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        type='link'
                        url={antiadbUrl(`/service/${this.props.id}/label/${this.props.id}`)}
                        mix={{
                            block: 'dashboard',
                            elem: 'item-action-button'
                        }}>
                        {i18n('service-page', 'menu-configs')}
                    </Button>
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='normal'
                        type='link'
                        url={antiadbUrl(`/service/${this.props.id}/${ACTIONS.UTILS}`)}
                        mix={{
                            block: 'dashboard',
                            elem: 'item-action-button'
                        }}>
                        {i18n('service-page', 'menu-utils')}
                    </Button>
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='normal'
                        type='link'
                        url={antiadbUrl(`/service/${this.props.id}/${ACTIONS.INFO}`)}
                        mix={{
                            block: 'dashboard',
                            elem: 'item-action-button'
                        }}>
                        {i18n('service-page', 'menu-info')}
                    </Button>
                </Bem>
            </Bem>
        );
    }
}

DashboardItem.propTypes = {
    id: PropTypes.string.isRequired,
    name: PropTypes.string,
    status: PropTypes.PropTypes.oneOf(Object.values(STATUS))
};

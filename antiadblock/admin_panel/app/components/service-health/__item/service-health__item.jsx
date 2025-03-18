import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import Link from 'lego-on-react/src/components/link/link.react';
import Dropdown from 'lego-on-react/src/components/dropdown/dropdown.react';
import Menu from 'lego-on-react/src/components/menu/menu.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import {setServiceHealtInProgress} from 'app/actions/health';

import Bem from 'app/components/bem/bem';
import Hint from 'app/components/hint/hint';

import i18n from 'app/lib/i18n';

import './service-health__item.css';

class ServiceHealthItem extends React.Component {
    constructor(props) {
        super(props);

        this._switcherRef = null;

        this.getScope = this.getScope.bind(this);
        this.onClick = this.onClick.bind(this);
        this.onProgressClick = this.onProgressClick.bind(this);
        this.setSwitcherRef = this.setSwitcherRef.bind(this);
    }

    setSwitcherRef(ref) {
        this._switcherRef = ref;
    }

    getScope() {
        return this.props.scope && this.props.scope.dom;
    }

    onClick(event) {
        const deprecatedClasses = ['popup2', 'dropdown2', 'hint'];

        let
            current = event.target,
            isDeprecated = false;

        while (!isDeprecated && current) {
            for (let i = 0; i < deprecatedClasses.length; i++) {
                if (current.classList.contains(deprecatedClasses[i])) {
                    isDeprecated = true;
                    break;
                }
            }
            current = current.parentElement;
        }

        // TODO костыль для показа хинта и менюшки, тк отменить пропогейшн нельзя
        if (!isDeprecated) {
            window.open(this.props.externalUrl, '_blank');
        }
    }

    onProgressClick(_, hours) {
        this.props.setInProgress(this.props.serviceId, this.props.checkId, hours);

        // TODO Костыль для скрытия менюшки после клика
        if (this._switcherRef) {
            this._switcherRef._onSwitcherClick();
        }
    }

    buildDate(dateStr) {
        const date = new Date(dateStr);

        return `${date.toLocaleDateString()} ${date.toLocaleTimeString()}`;
    }

    renderInProgressString() {
        const progressInfo = this.props.progressInfo || {};

        if (this.props.inProgress && (new Date(progressInfo.time_to).getTime() > Date.now())) {
            let timeTo = '';

            if (progressInfo.time_to) {
                timeTo = ` ${i18n('health', 'time-to')} ${this.buildDate(progressInfo.time_to)}`;
            }

            return `${i18n('health', 'at-work')}: ${progressInfo.login || 'unknown'}${timeTo}`;
        }

        return this.renderStatusString();
    }

    renderStatusString() {
        const {
            lastUpdate,
            transitionTime,
            state
        } = this.props;

        if (state !== 'green') {
            return `${i18n('health', 'transition-time')}: ${this.buildDate(transitionTime)}`;
        }

        if (lastUpdate) {
            return `${i18n('health', 'checked')}: ${this.buildDate(lastUpdate)}`;
        }

        return `${i18n('health', 'out-of-date')}`;
    }

    render() {
        const {
            title,
            description,
            hint,
            state,
            isValid,
            inProgress
        } = this.props;

        const progressInfo = this.props.progressInfo || {};
        const isInProgress = inProgress && (new Date(progressInfo.time_to).getTime() > Date.now());

        const statusString = this.renderStatusString();
        const inProgressString = this.renderInProgressString();

        return (
            <Link
                mix={[{
                    block: 'service-health-item',
                    mods: {
                        status: state,
                        disabled: isValid,
                        inprogress: isInProgress
                    }
                }]}
                theme='default'
                onClick={this.onClick}>
                <Bem
                    block='service-health-item'
                    elem='title'>
                    <Bem
                        block='service-health-item'
                        elem='hint'>
                        {hint ?
                            <Hint
                                text={hint}
                                to={['top', 'right']}
                                scope={this.props.scope} /> :
                            null }
                    </Bem>
                    <Bem
                        block='service-health-item'
                        elem='text'>
                        {title}
                    </Bem>
                    <Dropdown
                        view='default'
                        tone='default'
                        theme='normal'
                        size='xs'
                        switcher='link'
                        ref={this.setSwitcherRef}
                        mix={[{
                            block: 'service-health-item',
                            elem: 'dropdown'
                        }]}
                        popup={
                            <Dropdown.Popup
                                scope={this.getScope()}
                                directions={['bottom-center']}>
                                <Menu
                                    theme='normal'
                                    tone='default'
                                    view='default'
                                    size='xs'
                                    width='max'
                                    onClick={this.onProgressClick}
                                    mix={[{
                                        block: 'service-health-item',
                                        elem: 'menu'
                                    }]}>
                                    <Menu.Item disabled>
                                        {i18n('health', 'in-progress')}
                                    </Menu.Item>
                                    <Menu.Item
                                        type='link'
                                        val={168}>
                                        {i18n('health', 'seven-days')}
                                    </Menu.Item>
                                    <Menu.Item
                                        type='link'
                                        val={24}>
                                        {i18n('health', 'one-day')}
                                    </Menu.Item>
                                    <Menu.Item
                                        type='link'
                                        val={8}>
                                        {i18n('health', 'eight-hours')}
                                    </Menu.Item>
                                    <Menu.Item
                                        type='link'
                                        val={2}>
                                        {i18n('health', 'two-hours')}
                                    </Menu.Item>
                                    <Menu.Item
                                        type='link'
                                        val={1}>
                                        {i18n('health', 'one-hour')}
                                    </Menu.Item>
                                </Menu>
                            </Dropdown.Popup>
                        }
                        hasTail>
                        <Dropdown.Switcher>
                            <Icon glyph='type-arrow' />
                        </Dropdown.Switcher>
                    </Dropdown>
                </Bem>
                <Bem
                    key='description'
                    block='service-health-item'
                    elem='description'>
                    {description}
                </Bem>
                <Bem
                    key='action'
                    block='service-health-item'
                    elem='action'>
                    <Bem
                        block='service-health-item'
                        elem='status'>
                        <Bem
                            block='service-health-item'
                            elem='date'>
                            <span title={statusString}>{statusString}</span>
                        </Bem>
                        <Bem
                            block='service-health-item'
                            elem='in-progress'>
                            <span title={inProgressString}>{inProgressString}</span>
                        </Bem>
                    </Bem>
                </Bem>
            </Link>
        );
    }
}

ServiceHealthItem.propTypes = {
    checkId: PropTypes.string,
    serviceId: PropTypes.string,
    title: PropTypes.string,
    description: PropTypes.string,
    hint: PropTypes.string,
    state: PropTypes.string,
    externalUrl: PropTypes.string,
    isValid: PropTypes.bool,
    lastUpdate: PropTypes.string,
    transitionTime: PropTypes.string,
    inProgress: PropTypes.bool,
    progressInfo: PropTypes.object,
    setInProgress: PropTypes.func,
    scope: PropTypes.object
};

export default connect(null, dispatch => {
    return {
        setInProgress: (serviceId, checkId, hours) => {
            return dispatch(setServiceHealtInProgress(serviceId, checkId, hours));
        }
    };
})(ServiceHealthItem);

import React from 'react';
import {connect} from 'react-redux';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Table from 'app/components/table/table';
import Link from 'lego-on-react/src/components/link/link.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import ShortcutGroup from 'app/components/shortcut-group/shortcut-group';

import {changeLogVisibility} from 'app/actions/service/sbs';
import {getSbsScreenshotsChecksGroupVisibility} from 'app/reducers/service';
import {getService} from 'app/reducers';
import {sbsScreenshotsLogs} from 'app/types';

import i18n from 'app/lib/i18n';
import uniq from 'lodash/uniq';

import './service-sbs-screenshots-checks__log.css';
import 'app/components/icon/_theme/icon_theme_link-external.css';

// пока выпилили:  'balancer', 'nginx', 'cryprox',
const orderLogKeys = ['logs', 'cookies', 'blocked', 'console'];
const orderConsoleKeys = ['SEVERE', 'WARNING'];
const innerMultyOrderKeys = ['all', 'actions', 'response_codes', 'http_codes', 'methods'];

export class ServiceSbsScreenshotsChecksLog extends React.Component {
    constructor() {
        super();

        this.onChangeGroupVisible = this.onChangeGroupVisible.bind(this);
    }

    getOrderForMultiTable(key) {
        switch (key) {
            case 'all':
                return ['items'];
            default:
                return [];
        }
    }

    renderSubgroup(leftData, rightData, key, withPopup) {
        return (
            <Bem
                block='service-sbs-screenshots-checks-log'
                elem='subgroup'>
                <Bem
                    block='service-sbs-screenshots-checks-log'
                    elem='left'>
                    {leftData && (leftData.title && Object.keys(leftData.title).length) ?
                        <Bem
                            block='service-sbs-screenshots-checks-log'
                            elem='table'
                            mods={{
                                [key.toLowerCase()]: true
                            }}>
                            <Table
                                head={leftData.title}
                                body={Array.isArray(leftData.body) ? leftData.body : [leftData.body]}
                                order={this.getOrderForMultiTable(key)}
                                popup={withPopup} />
                        </Bem> : null}
                </Bem>
                <Bem
                    block='service-sbs-screenshots-checks-log'
                    elem='right'>
                    {rightData && (rightData.title && Object.keys(rightData.title).length) ?
                        <Bem
                            block='service-sbs-screenshots-checks-log'
                            elem='table'
                            mods={{
                                [key.toLowerCase()]: true
                            }}>
                            <Table
                                head={rightData.title}
                                body={Array.isArray(rightData.body) ? rightData.body : [rightData.body]}
                                order={this.getOrderForMultiTable(key)}
                                popup={withPopup} />
                        </Bem> : null}
                </Bem>
            </Bem>
        );
    }

    renderTitle(title, data, showShortcut) {
        const status = data.status ? data.status.split('_')[0] : '';

        return [
            data.url ?
                this.renderLink(this.renderTitleWithStatus(title, status), data.url) :
                this.renderTitleWithStatus(title, status),
            showShortcut && this.renderShortCutButton(title)
        ];
    }

    preparedBsLogData(data) {
        return {
            ...data,
            title: Object.keys(data.title).reduce((acc, key) => {
                const url = data.title[key].url;
                const status = data.title[key].status;

                return {
                    ...acc,
                    [key]: url ?
                        this.renderLink(this.renderTitleWithStatus(key, status), url) :
                        this.renderTitleWithStatus(key, status)
                };
            }, {})
        };
    }

    switchRenderLog(title, leftData, rightData) {
        switch (title) {
            case 'balancer':
            case 'nginx':
            case 'cryprox':
                return uniq(
                    innerMultyOrderKeys,
                    Object.keys(leftData.body),
                    Object.keys(rightData.body)
                ).map(key => (
                        (leftData.body[key] || rightData.body[key]) &&
                        this.renderSubgroup(leftData.body[key], rightData.body[key], key)
                ));
            case 'console':
                return uniq(
                    orderConsoleKeys,
                    Object.keys(leftData.body),
                    Object.keys(rightData.body)
                ).map(key => (
                    (leftData.body[key] || rightData.body[key]) &&
                    this.renderSubgroup(leftData.body[key], rightData.body[key], key, true)
                ));
            case 'logs':
                return this.renderSubgroup(this.preparedBsLogData(leftData), this.preparedBsLogData(rightData), title);
            default:
                return this.renderSubgroup(leftData, rightData, title, true);
        }
    }

    renderGroupLog(title, leftData = {}, rightData = {}) {
        return (
            <Bem
                block='service-sbs-screenshots-checks-log'
                elem='group'
                mods={{
                    checked: this.props.logsVisible[title]
                }}>
                <Bem
                    block='service-sbs-screenshots-checks-log'
                    elem='title'>
                    <Bem
                        block='service-sbs-screenshots-checks-log'
                        elem='title-left'>
                        {this.renderTitle(title, leftData)}
                    </Bem>
                    <Bem
                        block='service-sbs-screenshots-checks-log'
                        elem='title-right'>
                        {this.renderTitle(title, rightData, true)}
                    </Bem>
                </Bem>
                <Bem
                    block='service-sbs-screenshots-checks-log'
                    elem='body'
                    mods={{
                        visible: this.props.logsVisible[title]
                    }}>
                    {this.switchRenderLog(title, leftData, rightData)}
                </Bem>
            </Bem>
        );
    }

    renderLink(value, url) {
        return (
            <Link
                theme='normal'
                url={url}
                target='_blank'>
                {value}
                <Icon
                    size='s'
                    mix={[{
                        block: 'icon',
                        mods: {
                            theme: 'link-external'
                        }
                    }, {
                        block: 'service-sbs-screenshots-checks-log',
                        elem: 'icon-link'
                    }]} />
            </Link>

        );
    }

    onChangeGroupVisible(key) {
        return () => {
            this.props.changeLogVisibility(key, !this.props.logsVisible[key]);
        };
    }

    renderShortCutButton(key) {
        const groupIsVisible = this.props.logsVisible[key];

        return (
            <Button
                view='classic'
                tone='grey'
                type='check'
                size='xs'
                theme='normal'
                pin='brick-round'
                width='auto'
                mix={{
                    block: 'service-sbs-screenshots-checks-log',
                    elem: 'shortcut-button',
                    mods: {
                        checked: groupIsVisible
                    }
                }}
                checked={groupIsVisible}
                iconRight={{
                    mods: {
                        type: 'arrow',
                        direction: groupIsVisible ? 'down' : 'right'
                    }
                }}
                onClick={this.onChangeGroupVisible(key)}>
                {`${' '}`}
            </Button>
        );
    }

    renderTitleWithStatus(title, status) {
        return [
            title,
            status &&
            <span
                key='service-sbs-screenshots-checks-log__status'
                className='service-sbs-screenshots-checks-log__status'>
                [<span className={`service-sbs-screenshots-checks-log__status_${status}`}>{status}</span>]
            </span>
        ];
    }

    render() {
        const leftLogs = this.props.leftLogs;
        const rightLogs = this.props.rightLogs;

        return (
            <Bem
                block='service-sbs-screenshots-checks-log'
                mods={{
                    problem: this.props.hasProblem
                }}>
                <ShortcutGroup
                    title={i18n('sbs', 'show-info')}>
                    {orderLogKeys.map(key => (this.renderGroupLog(key, leftLogs[key], rightLogs[key])))}
                </ShortcutGroup>
            </Bem>
        );
    }
}

ServiceSbsScreenshotsChecksLog.propTypes = {
    leftLogs: sbsScreenshotsLogs,
    rightLogs: sbsScreenshotsLogs,
    changeLogVisibility: PropTypes.func.isRequired,
    logsVisible: PropTypes.object.isRequired,
    hasProblem: PropTypes.bool
};

export default connect(state => ({
    logsVisible: getSbsScreenshotsChecksGroupVisibility(getService(state))
}), dispatch => ({
    changeLogVisibility: (key, value) => {
        dispatch(changeLogVisibility(key, value));
    }
}))(ServiceSbsScreenshotsChecksLog);

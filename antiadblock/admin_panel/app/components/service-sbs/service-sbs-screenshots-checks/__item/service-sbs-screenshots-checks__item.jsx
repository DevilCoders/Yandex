import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ServiceSbsScreenshotsChecksLog from '../__log/service-sbs-screenshots-checks__log';
import LinkCopy from 'app/components/link-copy/link-copy';
import Image from 'lego-on-react/src/components/image/image.react';
import Link from 'lego-on-react/src/components/link/link.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import Popup from 'lego-on-react/src/components/popup/popup.react';

import i18n from 'app/lib/i18n';
import get from 'lodash/get';

import './service-sbs-screenshots-checks__item.css';
import 'app/components/icon/_theme/icon_theme_link-external.css';
import 'app/components/icon/_theme/icon_theme_link.css';

export default class ServiceSbsScreenshotsChecksItem extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            visible: false
        };
        this._anchor = null;
        this.setAnchorRef = this.setAnchorRef.bind(this);
        this.onChangeVisible = this.onChangeVisible.bind(this);
        this.onChangeFilter = this.onChangeFilter.bind(this);
        this.onShowModal = this.onShowModal.bind(this);
    }

    onChangeVisible() {
        if (!this.state.visible) {
            document.addEventListener('wheel', this.onChangeVisible);
        } else {
            document.removeEventListener('wheel', this.onChangeVisible);
        }

        this.setState(state => ({
            visible: !state.visible
        }));
    }

    renderInfo(curLog, otherLog) {
        const isDetectFailed = (curLog.adblocker !== 'Without adblock' && curLog.adblocker !== 'Without adblock crypted' && !curLog.ludca) ? 'failed' : false;
        const countActiveRules = (curLog.adblocker !== 'Without adblock' && curLog.adblocker !== 'Without adblock crypted') && Object.keys(curLog.rules || {}).length;
        const isProblem = curLog.has_problem;
        const countConsoleErrors = curLog.logs.console.body.SEVERE && curLog.logs.console.body.SEVERE.body.length;
        const elasticAuction = get(curLog, 'logs.elastic-auction.body.all.items', undefined);
        const elasticCount = get(curLog, 'logs.elastic-count.body.all.items', undefined);
        const keyLogs = ['bs-dsp-log', 'bs-event-log', 'bs-hit-log'];
        const bsLogs = keyLogs.some(key => (curLog.logs.logs.body[key]));
        const isVisible = !isNaN(elasticCount) || !isNaN(elasticAuction) || isDetectFailed ||
            countConsoleErrors || bsLogs || countActiveRules || curLog.has_problem;
        const infoData = [{
            key: 'problem',
            value: isProblem,
            customClass: `service-sbs-screenshots-checks-item__info_${isProblem}`
        }, {
            key: 'detect',
            value: isDetectFailed
        }, {
            key: 'rules',
            value: countActiveRules
        }, {
            key: 'errors',
            value: countConsoleErrors
        }, {
            key: 'elastic-auction',
            value: elasticAuction,
            customClass: ''
        }, {
            key: 'elastic-count',
            value: elasticCount,
            customClass: ''
        }];

        return (
            isVisible ?
                <Bem
                    block='service-sbs-screenshots-checks-item'
                    elem='info'>
                    {infoData.map(item => (
                        this.renderInfoBlock(item.key, item.value, item.customClass)
                    ))}
                    {bsLogs && ['bs-dsp-log', 'bs-event-log', 'bs-hit-log'].map(key => {
                        const curItems = curLog.logs.logs.body[key];
                        const otherItems = otherLog.logs.logs.body[key];

                        return (
                            this.renderInfoBlock(key, curItems, curItems === otherItems && '')
                        );
                    })}
                </Bem> : null
        );
    }

    renderInfoBlock(key, value, customClass) {
        if (value === undefined || value === null || value === false) {
            return null;
        }

        return (
            <Bem
                block='service-sbs-screenshots-checks-item'
                elem={`info-${key}`}
                key={`info-${key}`}>
                {i18n('sbs', key)}:<span className={typeof customClass === 'string' ? customClass : 'service-sbs-screenshots-checks-item__info_warning'}>{value}</span>
            </Bem>
        );
    }

    onChangeFilter(key, val) {
        return () => {
            this.props.onChangeFilter(key, val);
        };
    }

    renderDirectionItem(data, direction, otherData) {
        return (
            <Bem
                block='service-sbs-screenshots-checks-item'
                elem={direction}>
                <Bem
                    block='service-sbs-screenshots-checks-item'
                    elem={`description-${direction}`}
                    mix={data.has_problem ? {
                        block: 'service-sbs-screenshots-checks-item',
                        elem: 'status',
                        mods: {
                            [data.has_problem]: true
                        }
                    } : null}>
                    <Bem
                        block='service-sbs-screenshots-checks-item'
                        elem='description-tag'
                        onClick={this.onChangeFilter('byBrowser', data.browser)}>
                        {data.browser}
                    </Bem>
                    <Bem
                        block='service-sbs-screenshots-checks-item'
                        elem='description-tag'
                        onClick={this.onChangeFilter('byBlocker' + direction[0].toUpperCase() + direction.slice(1), data.adblocker)}>
                        {data.adblocker}
                    </Bem>
                </Bem>
                <Bem
                    block='service-sbs-screenshots-checks-item'
                    elem='image'>
                    <Image
                        url={data.img_url}
                        alt={`${data.browser} - ${data.adblocker}`} />
                    {this.renderInfo(data, otherData)}
                </Bem>
            </Bem>
        );
    }

    isProblem(left, right) {
        const IS_PROBLEM = 'problem';

        return left.has_problem === IS_PROBLEM || right.has_problem === IS_PROBLEM;
    }

    setAnchorRef(ref) {
        this._anchor = ref;
    }

    getCaseUrl() {
        const {pair} = this.props;

        return location.origin + location.pathname + encodeURI('?byBrowser=' + pair.left.browser + '&byBlockerLeft=' +
            pair.left.adblocker + '&byBlockerRight=' + pair.right.adblocker) + '&byUrl=' + encodeURIComponent(pair.left.url);
    }

    getCaseRules() {
        const {left, right} = this.props.pair;
        const uniqRules = new Set([
            ...(Object.keys(left.rules || {})),
            ...(Object.keys(right.rules || {}))
        ]);

        return Array.from(uniqRules).join('\n');
    }

    onShowModal() {
        const data = {
            left: {
                ...this.props.pair.left,
                logs: {
                    ...(this.props.pair.left.logs || {}),
                    cookies: null,
                    console: null
                }
            },
            right: {
                ...this.props.pair.right,
                logs: {
                    ...(this.props.pair.right.logs || {}),
                    cookies: null,
                    console: null
                }
            }
        };

        this.props.onChangeModalVisible(data);
    }

    renderKebab() {
        return (
            <Bem
                block='service-sbs-screenshots-checks-item'
                elem='kebab'
                mods={{
                    active: this.state.visible
                }}
                onClick={this.onChangeVisible}
                tagRef={this.setAnchorRef}>
                <Bem block=''>
                    <svg width='8' height='20' xmlns='http://www.w3.org/2000/svg'>
                        <circle cx='4' cy='4' r='1.5' />
                        <circle cx='4' cy='10' r='1.5' />
                        <circle cx='4' cy='16' r='1.5' />
                    </svg>
                </Bem>
                <Popup
                    hasTail
                    anchor={this._anchor}
                    directions={['left-center', 'left-bottom', 'left-top', 'right-center', 'right-bottom', 'right-top']}
                    autoclosable
                    onOutsideClick={this.onChangeVisible}
                    onClose={this.onChangeVisible}
                    visible={this.state.visible}
                    theme='normal'>
                    <Bem
                        block='service-sbs-screenshots-checks-item'
                        elem='popup-container'>
                        <Bem
                            block='service-sbs-screenshots-checks-item'
                            elem='popup-container-item'>
                            <LinkCopy
                                disableHint
                                value={this.props.pair.left.url}
                                onClick={this.onChangeVisible}
                                description={i18n('sbs', 'copy-page-url')} />
                        </Bem>
                        <Bem
                            block='service-sbs-screenshots-checks-item'
                            elem='popup-container-item'>
                            <LinkCopy
                                disableHint
                                value={this.getCaseUrl()}
                                onClick={this.onChangeVisible}
                                description={i18n('sbs', 'copy-case-url')} />
                        </Bem>
                        <Bem
                            block='service-sbs-screenshots-checks-item'
                            elem='popup-container-item'>
                            <Link
                                theme='pseudo'
                                onClick={this.onShowModal}>
                                {i18n('sbs', 'show-case-info')}
                            </Link>
                        </Bem>
                        <Bem
                            block='service-sbs-screenshots-checks-item'
                            elem='popup-container-item'>
                            <LinkCopy
                                disableHint
                                value={this.getCaseRules()}
                                onClick={this.onChangeVisible}
                                description={i18n('sbs', 'copy-case-rules')} />
                        </Bem>
                    </Bem>
                </Popup>
            </Bem>
        );
    }

    render() {
        const {
            left,
            right
        } = this.props.pair;
        const clearUrl = left.url.replace(/(^\w+:|^)\/\//, '').replace(/(\/)$/, '');
        const itemKey = `${left.browser}${left.adblocker}${left.start}-${right.browser}${right.adblocker}${right.start}`;
        const hasProblem = this.isProblem(left, right);

        return (
            <Bem
                key={itemKey}
                block='service-sbs-screenshots-checks-item'>
                <Bem
                    block='service-sbs-screenshots-checks-item'
                    elem='content'
                    mods={{
                        problem: hasProblem
                    }}>
                    <Bem
                        block='service-sbs-screenshots-checks-item'
                        elem='title'>
                        <Bem
                            block='service-sbs-screenshots-checks-item'
                            elem='title-description'>
                            <Link
                                theme='normal'
                                url={left.url}
                                target='_blank'>
                                {clearUrl}
                                <Icon
                                    size='s'
                                    mix={[{
                                        block: 'icon',
                                        mods: {
                                            theme: 'link-external'
                                        }
                                    }, {
                                        block: 'service-sbs-screenshots-checks-item',
                                        elem: 'icon-link'
                                    }]} />
                            </Link>
                        </Bem>
                        {this.renderKebab()}
                    </Bem>
                    <Bem
                        block='service-sbs-screenshots-checks-item'
                        elem='cmp'>
                        {this.renderDirectionItem(left, 'left', right)}
                        <Bem
                            block='service-sbs-screenshots-checks-item'
                            elem='space'>
                            <Bem
                                block='service-sbs-screenshots-checks-item'
                                elem='line' />
                        </Bem>
                        {this.renderDirectionItem(right, 'right', right)}
                    </Bem>
                </Bem>
                <ServiceSbsScreenshotsChecksLog
                    leftLogs={left.logs}
                    rightLogs={right.logs}
                    hasProblem={hasProblem} />
            </Bem>
        );
    }
}

ServiceSbsScreenshotsChecksItem.propTypes = {
    pair: PropTypes.any,
    onChangeFilter: PropTypes.func.isRequired,
    onChangeModalVisible: PropTypes.func.isRequired
};

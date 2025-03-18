import React from 'react';

import {Table, THead, TBody, Td, Th, Tr} from 'app/components/table/table-components';
import Link from 'lego-on-react/src/components/link/link.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import {serviceRulesSchemaType, serviceRulesDataItemsType} from 'app/types';

import i18n from 'app/lib/i18n';

import './service-rules__table.css';
import 'app/components/icon/_theme/icon_theme_link-external.css';

const orderTableKeys = ['added', 'raw_rule', 'list_url'];
const urlTitleEnum = {
    'filters.adtidy.org/extension/firefox/filters/1.txt': 'Adguard_Ru - Firefox',
    'filters.adtidy.org/extension/chromium/filters/1.txt': 'Adguard_Ru - Chrome',
    'easylist-downloads.adblockplus.org/advblock+cssfixes.txt': 'Ru AdList + CSS Fixes',
    'easylist-downloads.adblockplus.org/ruadlist+easylist.txt': 'Ru AdList + EasyList',
    'dl.opera.com/download/get/?adblocker=adlist&country=ru': 'Opera',
    'easylist-downloads.adblockplus.org/cntblock.txt': 'RU AdList: Counters',
    'easylist-downloads.adblockplus.org/abp-filters-anti-cv.txt': 'ABP filters',
    'filters.adtidy.org/extension/chromium/filters/2.txt': 'Adguard_En - Chrome + EasyList',
    'filters.adtidy.org/extension/firefox/filters/2.txt': 'Adguard_En - Firefox + EasyList',
    'raw.githubusercontent.com/AdguardTeam/FiltersRegistry/master/filters/filter_11_Mobile/filter.txt': 'Adguard - Mobile'
};

class ServiceRules extends React.Component {
    renderHeadTableCell(key, mods = {}) {
        const {
            schema
        } = this.props;

        return (
            <Th
                block='service-rules-table'
                elem='cell'
                mods={{
                    header: true,
                    ...mods
                }}
                key={schema[key]}>
                {i18n('service-rules', schema[key])}
            </Th>
        );
    }

    getShortTitleForUrl(url) {
        return urlTitleEnum[url] || url;
    }

    renderHeadTable() {
        return (
            <THead
                block='service-rules-table'
                elem='header'>
                <Tr
                    block='service-rules-table'
                    elem='row'
                    mods={{
                        header: true
                    }}>
                    {orderTableKeys.map(key => {
                        let cell = null;

                        switch (key) {
                            case 'list_url': {
                                cell = this.renderHeadTableCell(key, {fsize150: true});
                                break;
                            }
                            case 'added': {
                                cell = this.renderHeadTableCell(key, {fsize70: true});
                                break;
                            }
                            default:
                                cell = this.renderHeadTableCell(key);
                                break;
                        }
                        return cell;
                    })}
                </Tr>
            </THead>
        );
    }

    renderExternalLink(url, target) {
        return (
            <Link
                key={url}
                theme='normal'
                url={`https://${url}`}
                hasIcon
                target={target}
                iconLeft={
                    <Icon
                        size='s'
                        mix={{
                            block: 'icon',
                            mods: {
                                theme: 'link-external'
                            }
                        }} />
                }
                text={this.getShortTitleForUrl(url)} />
        );
    }

    renderBodyTableSimpleCell(data, mods = {}) {
        return (
            <Td
                key={`body-cell-${data}`}
                block='service-rules-table'
                elem='cell'
                mods={{
                    body: true,
                    ...mods
                }}>
                {data}
            </Td>
        );
    }

    renderBodyTableLinksCell(data, mods = {}) {
        return (
            <Td
                key={`body-cell-${data}`}
                block='service-rules-table'
                elem='cell'
                mods={{
                    body: true,
                    ...mods
                }}>
                {data.map(item => (
                    this.renderExternalLink(item, '_blank')
                ))}
            </Td>
        );
    }

    renderBodyTableRow(item) {
        return (
            <Tr
                key={`body-row-${item.raw_rule}`}
                block='service-rules-table'
                elem='row'
                mods={{
                    body: true
                }}>
                {orderTableKeys.map(key => {
                    let cell = null;

                    switch (key) {
                        case 'list_url': {
                            cell = this.renderBodyTableLinksCell(item[key], {ontop: true});
                            break;
                        }
                        default:
                            cell = this.renderBodyTableSimpleCell(item[key]);
                            break;
                    }

                    return cell;
                })}
            </Tr>
        );
    }

    renderBodyTable() {
        const {
            data
        } = this.props;

        return (
            <TBody
                block='service-rules-table'
                elem='body'>
                {data.map(item => (
                    this.renderBodyTableRow(item)
                ))}
            </TBody>
        );
    }

    render() {
        return (
            <Table
                block='service-rules-table'>
                {this.renderHeadTable()}
                {this.renderBodyTable()}
            </Table>
        );
    }
}

ServiceRules.propTypes = {
    schema: serviceRulesSchemaType,
    data: serviceRulesDataItemsType
};

export default ServiceRules;

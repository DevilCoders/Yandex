import React from 'react';
import PropTypes from 'prop-types';

import Link from 'lego-on-react/src/components/link/link.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import {Table, THead, TBody, Td, Th, Tr} from 'app/components/table/table-components';

import i18n from 'app/lib/i18n';
import union from 'lodash/union';
import {antiadbUrl} from 'app/lib/url';
import {formatDateTo} from 'app/lib/date';

import {RUN_ID_STATUS} from 'app/enums/sbs';

import './table-sbs-result-checks.css';
import 'app/components/icon/_theme/icon_theme_link-external.css';
import 'app/components/icon/_theme/icon_theme_arrow-up-down.css';
import 'app/components/icon/_theme/icon_theme_arrow-up.css';
import 'app/components/icon/_theme/icon_theme_arrow-down.css';

const sandboxUrl = 'https://sandbox.yandex-team.ru/task/';
const orderTableKeys = ['id', 'sandbox_id', 'config_id', 'profile_id', 'owner', 'status', 'date'];

class TableSbsResultChecks extends React.Component {
    getSbsScreenshotsChecksDiffUrl(runId) {
        const {
            serviceId
        } = this.props;

        return antiadbUrl(`/service/${serviceId}/screenshots-checks/diff/${runId}/${runId}`);
    }

    renderHeadTableCell(key) {
        const {
            schema,
            onChangeSortedBy,
            isReverseSorted,
            sortedBy
        } = this.props;

        let arrowTheme = 'up-down';

        if (sortedBy === key) {
            arrowTheme = 'down';
            if (isReverseSorted) {
                arrowTheme = 'up';
            }
        }

        return (
            <Th
                block='table-sbs-result-checks'
                elem='cell'
                mods={{
                    header: true
                }}
                key={schema[key]}
                onClick={onChangeSortedBy(key)}>
                {i18n('sbs', schema[key]) || schema[key]}
                <Icon
                    size='s'
                    mix={[{
                        block: 'icon',
                        mods: {
                            theme: `arrow-${arrowTheme}`
                        }}, {
                            block: '',
                            elem: 'icon'
                    }]} />
            </Th>
        );
    }

    renderHeadTable() {
        const {
            schema
        } = this.props;
        const keysSchema = union(orderTableKeys, Object.keys(schema));

        return (
            <THead
                block='table-sbs-result-checks'
                elem='header'>
                <Tr
                    block='table-sbs-result-checks'
                    elem='row'
                    mods={{
                        header: true
                    }}>
                    {keysSchema.map(key => (
                        this.renderHeadTableCell(key)
                    ))}
                </Tr>
            </THead>
        );
    }

    renderBodyTableSimpleCell(data, key, mods = {}) {
        return (
            <Td
                key={`body-cell-${key}`}
                block='table-sbs-result-checks'
                elem='cell'
                mods={{
                    body: true,
                    ...mods
                }}>
                {i18n('sbs', data) || data}
            </Td>
        );
    }

    renderBodyTablePseudoLink(data, key, onChange) {
        return (
            <Td
                key={`body-cell-${key}`}
                block='table-sbs-result-checks'
                elem='cell'
                mods={{
                    body: true
                }}>
                <Link
                    theme='normal'
                    elem='item-id-link'
                    onClick={onChange}>
                    #{data}
                </Link>
            </Td>
        );
    }

    renderBodyTableLinkCell(data, key, url, target) {
        return (
            <Td
                key={`body-cell-${key}`}
                block='table-sbs-result-checks'
                elem='cell'
                mods={{
                    body: true
                }}>
                <Link
                    theme='normal'
                    url={url}
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
                    text={String(data)} />
            </Td>
        );
    }

    renderBodyTableRow(item) {
        const {
            id,
            owner,
            date,
            sandbox_id: sandboxId,
            profile_id: profileId,
            config_id: configId
        } = item;
        const status = item.status.replace('_', '-');
        const itemKeys = union(orderTableKeys, Object.keys(item));

        return (
            <Tr
                block='table-sbs-result-checks'
                elem='row'
                key={`row-${id}-${owner}`}
                mods={{
                    body: true
                }}>
                {itemKeys.map(key => {
                    let cell;

                    switch (key) {
                        case 'id':
                            cell = RUN_ID_STATUS.SUCCESS === item.status || RUN_ID_STATUS.FAIL === item.status ?
                                this.renderBodyTableLinkCell(id, `id${id}-${owner}`, this.getSbsScreenshotsChecksDiffUrl(id), '_blank') :
                                this.renderBodyTableSimpleCell(id, `id${id}-${owner}`);
                            break;
                        case 'date':
                            cell = this.renderBodyTableSimpleCell(formatDateTo(date, 'DD.MM.YYYY HH:mm:ss'), `date${date}-${owner}`);
                            break;
                        case 'sandbox_id':
                            cell = sandboxId ?
                                this.renderBodyTableLinkCell(sandboxId, `sandboxId${sandboxId}-${owner}`, `${sandboxUrl}${sandboxId}`, '_blank') :
                                this.renderBodyTableSimpleCell(sandboxId, `sandboxId${sandboxId}-${owner}`);
                            break;
                        case 'status':
                            cell = this.renderBodyTableSimpleCell(status, `status${status}-${owner}`, {
                                [`status-${status}`]: true
                            });
                            break;
                        case 'profile_id': {
                            cell = this.renderBodyTablePseudoLink(profileId, `profileId${profileId}-${owner}`, this.props.onOpenProfile(profileId, true));
                            break;
                        }
                        case 'config_id': {
                            cell = this.renderBodyTableSimpleCell(`#${configId}`, `configId${configId}-${owner}`);
                            break;
                        }
                        default:
                            cell = this.renderBodyTableSimpleCell(item[key], `default${item[key]}-${owner}`);
                            break;
                    }

                    return cell || this.renderBodyTableSimpleCell('empty');
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
                block='table-sbs-result-checks'
                elem='body'>
                {data.items.map(item => (
                    this.renderBodyTableRow(item)
                ))}
            </TBody>
        );
    }

    render() {
        return (
            <Table
                block='table-sbs-result-checks'>
                {this.renderHeadTable()}
                {this.renderBodyTable()}
            </Table>
        );
    }
}

TableSbsResultChecks.propTypes = {
    schema: PropTypes.object.isRequired,
    data: PropTypes.object.isRequired,
    onChangeSortedBy: PropTypes.func,
    isReverseSorted: PropTypes.bool,
    sortedBy: PropTypes.string,
    serviceId: PropTypes.string.isRequired,
    onOpenProfile: PropTypes.func.isRequired
};

export default TableSbsResultChecks;

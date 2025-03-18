import React from 'react';
import PropTypes from 'prop-types';

import {Table as TableComponent, TBody, Td, Th, THead, Tr} from './table-components';
import TableCellBlock from './table__cell-block/table__cell-block';
import union from 'lodash/union';

import './table.css';

class Table extends React.Component {
    renderHeadTableCell(title) {
        return (
            <Th
                block='table'
                elem='cell'
                mods={{
                    header: true
                }}>
                {title}
            </Th>
        );
    }

    renderBodyTableSimpleCell(data, mods = {}) {
        return (
            <Td
                block='table'
                elem='cell'
                mods={{
                    body: true,
                    ...mods
                }}>
                <TableCellBlock
                    popup={this.props.popup}
                    data={data} />
            </Td>
        );
    }

    renderBodyTableRow(data, order) {
        return (
            <Tr
                key={`body-row-${''}`}
                block='table'
                elem='row'
                mods={{
                    body: true
                }}>
                {order.map(key => (
                    this.renderBodyTableSimpleCell(data[key])
                ))}
            </Tr>
        );
    }

    renderBodyTable(order, data) {
        return (
            <TBody
                block='table'
                elem='body'>
                {data.map(value => (
                    this.renderBodyTableRow(value, order)
                ))}
            </TBody>
        );
    }

    renderHeadTable(order, head) {
        return (
            <THead
                block='table'
                elem='header'>
                <Tr
                    block='table'
                    elem='row'
                    mods={{
                        header: true
                    }}>
                    {order.map(key => (
                        this.renderHeadTableCell(head[key])
                    ))}
                </Tr>
            </THead>
        );
    }

    prepareOrder(order, head) {
        const headKeys = Object.keys(head);
        return order.length !== headKeys.length ? union(order, headKeys) : order;
    }

    render() {
        const order = this.prepareOrder(this.props.order, this.props.head);

        this.props.body.forEach(item => {
            const itemKeys = Object.keys(item);

            itemKeys.forEach(key => {
                if (Array.isArray(item[key])) {
                    item[key] = item[key].toString();
                } else if (typeof item[key] === 'object' && item !== null) {
                    item[key] = JSON.stringify(item[key]);
                }
            });
        });

        return (
            <TableComponent
                block='table'>
                {this.renderHeadTable(order, this.props.head)}
                {this.renderBodyTable(order, this.props.body)}
            </TableComponent>
        );
    }
}

Table.propTypes = {
    head: PropTypes.object,
    body: PropTypes.array,
    order: PropTypes.array,
    popup: PropTypes.bool
};

Table.defaultProps = {
    order: [],
    head: {},
    body: []
};

export default Table;

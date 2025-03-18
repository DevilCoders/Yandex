import React from 'react';
import PropTypes from 'prop-types';

import bemClassesCreator from 'app/lib/bem-classes-creator';

export class Tr extends React.Component {
    render() {
        const {
            block,
            elem,
            mods,
            mix,
            onClick,
            style,
            children
        } = this.props;

        return (
            <tr
                style={style}
                onClick={onClick}
                className={bemClassesCreator(block, elem, mods, mix)}>
                {children}
            </tr>
        );
    }
}

Tr.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    style: PropTypes.string,
    onClick: PropTypes.func,
    children: PropTypes.node
};

export class Th extends React.Component {
    render() {
        const {
            block,
            elem,
            mods,
            mix,
            onClick,
            style,
            children,
            colSpan,
            onMouseDown
        } = this.props;

        return (
            <th
                style={style}
                onClick={onClick}
                colSpan={colSpan}
                onMouseDown={onMouseDown}
                className={bemClassesCreator(block, elem, mods, mix)}>
                {children}
            </th>
        );
    }
}

Th.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    style: PropTypes.string,
    onClick: PropTypes.func,
    children: PropTypes.node,
    colSpan: PropTypes.number,
    onMouseDown: PropTypes.func
};

export class TBody extends React.Component {
    render() {
        const {
            block,
            elem,
            mods,
            mix,
            style,
            children
        } = this.props;

        return (
            <tbody
                style={style}
                className={bemClassesCreator(block, elem, mods, mix)}>
                {children}
            </tbody>
        );
    }
}

TBody.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    style: PropTypes.string,
    children: PropTypes.node
};

export class Td extends React.Component {
    render() {
        const {
            block,
            elem,
            mods,
            mix,
            style,
            children,
            onMouseEnter,
            onClick
        } = this.props;

        return (
            <td
                style={style}
                onMouseEnter={onMouseEnter}
                onClick={onClick}
                className={bemClassesCreator(block, elem, mods, mix)}>
                {children}
            </td>
        );
    }
}

Td.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    style: PropTypes.string,
    children: PropTypes.node,
    onMouseEnter: PropTypes.func,
    onClick: PropTypes.func
};

export class THead extends React.Component {
    render() {
        const {
            block,
            elem,
            mods,
            mix,
            style,
            children
        } = this.props;

        return (
            <thead
                style={style}
                className={bemClassesCreator(block, elem, mods, mix)}>
                {children}
            </thead>
        );
    }
}

THead.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    style: PropTypes.string,
    children: PropTypes.node
};

export class Table extends React.Component {
    render() {
        const {
            block,
            elem,
            mods,
            mix,
            style,
            children,
            tagRef
        } = this.props;

        return (
            <table
                style={style}
                ref={tagRef}
                className={bemClassesCreator(block, elem, mods, mix)}>
                {children}
            </table>
        );
    }
}

Table.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    style: PropTypes.string,
    children: PropTypes.node,
    tagRef: PropTypes.any
};

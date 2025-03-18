import React from 'react';
import PropTypes from 'prop-types';
import {omitBy, isNil} from 'lodash';

import bemClassesCreator from 'app/lib/bem-classes-creator';

export default class Bem extends React.Component {
    render() {
        const attrs = omitBy({
            style: this.props.style,
            onClick: this.props.onClick,
            ref: this.props.tagRef,
            className: bemClassesCreator(this.props.block, this.props.elem, this.props.mods, this.props.mix),
            onMouseDown: this.props.onMouseDown,
            onMouseUp: this.props.onMouseUp,
            'data-name': this.props['data-name']
        }, isNil);

        return (
            <div
                {...attrs} >
                {this.props.children}
            </div>
        );
    }
}

Bem.propTypes = {
    block: PropTypes.string.isRequired,
    elem: PropTypes.string,
    mods: PropTypes.object,
    mix: PropTypes.oneOfType([
        PropTypes.object,
        PropTypes.array
    ]),
    children: PropTypes.node,
    tagRef: PropTypes.func,
    onClick: PropTypes.func,
    style: PropTypes.object,
    onMouseDown: PropTypes.func,
    onMouseUp: PropTypes.func,
    'data-name': PropTypes.string
};

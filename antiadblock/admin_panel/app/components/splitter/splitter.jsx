import React from 'react';
import PropTypes from 'prop-types';
import throttle from 'lodash/throttle';

import Bem from 'app/components/bem/bem';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import './splitter.css';
import 'app/components/icon/_theme/icon_theme_ellipsis.css';

// можно сжимать только до этих значений
const MIN_LEFT_BLOCK_SIZE = 100;
const MIN_RIGHT_BLOCK_SIZE = 300;
const MARGIN_RESIZE = 8;
const DEFAULT_WIDTH_LEFT_BLOCK = 250;

export default class Splitter extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            widthLeftBlock:
                Number(localStorage.getItem(`${this.props.componentLocalStorageName}_splitter_width`)) ||
                this.props.leftWidth ||
                DEFAULT_WIDTH_LEFT_BLOCK
        };

        this.splitResize = null;
        this.leftBlock = null;
        this.isDown = false;
        this.leftOffset = null;

        this.onDown = this.onDown.bind(this);
        this.onUp = this.onUp.bind(this);
        this.onMove = throttle(this.onMove.bind(this), 25);
    }

    componentDidMount() {
        if (!this.splitResize) {
            this.splitResize = document.getElementsByClassName('splitter__resize')[0];
        }

        if (!this.leftBlock) {
            this.leftBlock = document.getElementsByClassName('splitter__left')[0];
            this.leftOffset = this.offsetLeftFull(this.leftBlock);
        }

        if (this.splitResize) {
            this.splitResize.addEventListener('mousedown', this.onDown);
            window.addEventListener('mouseup', this.onUp);
            window.addEventListener('mousemove', this.onMove);
        }
    }

    stopEvent(e) {
        if (e.stopPropagation) {
            e.stopPropagation();
        }

        if (e.preventDefault) {
            e.preventDefault();
        }

        e.cancelBubble = true;
        e.returnValue = false;
    }

    onDown(e) {
        this.isDown = true;
        this.stopEvent(e);
    }

    onUp() {
        this.isDown = false;
        localStorage.setItem(`${this.props.componentLocalStorageName}_splitter_width`, String(this.state.widthLeftBlock));
    }

    offsetLeftFull(element) {
        let offset = 0;

        while (element) {
            offset += element.offsetLeft;
            element = element.offsetParent;
        }

        return offset;
    }

    onMove(e) {
        if (this.isDown) {
            const value = e.x - this.leftOffset - MARGIN_RESIZE;

            if (
                value >= ((Number.isFinite(this.props.minLeftWidth) &&
                    this.props.minLeftWidth) || MIN_LEFT_BLOCK_SIZE) &&
                value <= ((Number.isFinite(this.props.maxLeftWidth) &&
                    this.props.maxLeftWidth) || MIN_RIGHT_BLOCK_SIZE)) {
                this.setState({
                    widthLeftBlock: value
                });
            }
        }
    }

    render() {
        return (
            <Bem
                block='splitter'>
                <Bem
                    block='splitter'
                    elem='left'
                    style={{
                        width: `${this.state.widthLeftBlock}px`
                    }}>
                    {this.props.leftComponent}
                </Bem>
                <Bem
                    block='splitter'
                    elem='resize'>
                    <Icon
                        key='icon'
                        mix={{
                            block: 'icon',
                            mods: {
                                theme: 'ellipsis'
                            }
                        }}
                        size='s' />
                </Bem>
                <Bem
                    block='splitter'
                    elem='right'>
                    {this.props.rightComponent}
                </Bem>
            </Bem>
        );
    }
}

Splitter.propTypes = {
    leftComponent: PropTypes.node.isRequired,
    rightComponent: PropTypes.node.isRequired,
    leftWidth: PropTypes.number,
    componentLocalStorageName: PropTypes.string,
    minLeftWidth: PropTypes.number.isRequired,
    maxLeftWidth: PropTypes.number
};

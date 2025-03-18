import React from 'react';
import PropTypes from 'prop-types';
import {Link} from 'react-router-dom';

import Icon from 'lego-on-react/src/components/icon/icon.react';
import Button from 'lego-on-react/src/components/button/button.react';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';

import throttle from 'lodash/throttle';

import './tree.css';
import 'app/components/icon/_theme/icon_theme_reorder.css';

class Tree extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            enabled: false,
            name: null,
            position: {
                startX: 0,
                startY: 0,
                currentX: 0,
                currentY: 0
            }
        };

        this.onMouseUp = this.onMouseUp.bind(this);
        this.onMouseMove = throttle(this.onMouseMove.bind(this), 50);
    }

    onMouseMove(event) {
        this.setState(state => ({
            position: {
                ...state.position,
                currentX: event.clientX,
                currentY: event.clientY
            }
        }));
    }

    onMouseUp(event) {
        event.preventDefault();
        event.stopPropagation();

        window.removeEventListener('mouseup', this.onMouseUp);
        window.removeEventListener('mousemove', this.onMouseMove);

        let current = event.target;

        while (current && !current.classList.contains('tree__item')) {
            current = current.parentElement;
        }

        if (current) {
            const name = current.dataset.name;
            if (name && this.props.onMove) {
                this.props.onMove(this.state.name, name);
            }
        }

        this.setState({
            enabled: false,
            name: null,
            position: {
                startX: 0,
                startY: 0,
                currentX: 0,
                currentY: 0
            }
        });
    }

    onAdd(item) {
        return event => {
            event.preventDefault();
            event.stopPropagation();
            return this.props.onAdd && this.props.onAdd(item);
        };
    }

    flattenTree() {
        const result = [];

        if (this.props.tree && this.props.tree.ROOT) {
            result.push({
                level: 0,
                name: 'ROOT',
                tree: this.props.tree.ROOT
            });

            let i = 0;

            while (i < result.length) {
                const current = result[i];
                const children = Object.keys(current.tree).map(name => ({
                    level: current.level + 1,
                    name: name,
                    tree: current.tree[name]
                }));
                result.splice(i + 1, 0, ...children);
                i++;
            }
        }

        return result;
    }

    onMouseDown(item) {
        return event => {
            event.preventDefault();
            event.stopPropagation();

            if (!this.state.enabled) {
                window.addEventListener('mouseup', this.onMouseUp);
                window.addEventListener('mousemove', this.onMouseMove);

                this.setState({
                    enabled: true,
                    name: item.name,
                    position: {
                        startX: event.clientX,
                        startY: event.clientY,
                        currentX: event.clientX,
                        currentY: event.clientY
                    }
                });
            }
        };
    }

    renderItem(item, isActive, isDnd) {
        const style = {};
        if (isDnd) {
            style.top = this.state.position.currentY - this.state.position.startY;
        }

        return (
            <Bem
                block='tree'
                elem='item'
                mods={{
                    dnd: isDnd,
                    active: isActive
                }}
                data-name={item.name} >
                <Icon glyph='type-check' />
                <Bem
                    block='tree'
                    elem='name'
                    style={{marginLeft: `${20 * item.level}px`}} >
                    {item.name}
                </Bem>
                <Bem
                    block='tree'
                    elem='icon'
                    onMouseDown={this.onMouseDown(item)}>
                    <Icon
                        size='s'
                        mix={{
                            block: 'icon',
                            mods: {
                                theme: 'reorder'
                            }}} />
                </Bem>
                <Button
                    mix={{
                        block: 'tree',
                        elem: 'addButton'
                    }}
                    theme='clear'
                    onClick={this.onAdd(item)}>
                    +
                </Button>
            </Bem>
        );
    }

    render() {
        return (
            <Bem block='tree'>
                {this.flattenTree().map(item => (
                    <Link
                        key={item.name}
                        className='tree__menu-item-wrapper'
                        to={`/service/${this.props.serviceId}/label/${item.name}`}>
                        {this.renderItem(item, item.name === this.props.activeId, item.name === this.state.name)}
                    </Link>
                ))}
                {this.props.loading ?
                    <Preloader key='preloader' /> :
                    null}
            </Bem>
        );
    }
}

Tree.propTypes = {
    tree: PropTypes.any,
    activeId: PropTypes.any,
    serviceId: PropTypes.string,
    loading: PropTypes.bool,
    onAdd: PropTypes.func,
    onMove: PropTypes.func
};

export default Tree;

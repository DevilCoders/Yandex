import React from 'react';
import PropTypes from 'prop-types';
import Bem from 'app/components/bem/bem';

export default class InfiniteListList extends React.Component {
    render() {
        let items = this.props.items || [];
        // TODO: миксы

        return (
            <Bem
                block='infinite-list'
                elem='list'>
                {[
                    this.props.children,
                    ...items.map(itemId =>
                        (
                            <this.props.Item
                                key={itemId}
                                id={itemId}
                                mix={{
                                    block: 'infinite-list',
                                    elem: 'item'
                                }}
                                scope={this.props.scope} />
                        )
                    )
                ]}
            </Bem>
        );
    }
}

InfiniteListList.propTypes = {
    items: PropTypes.array,
    children: PropTypes.array,
    scope: PropTypes.object
};

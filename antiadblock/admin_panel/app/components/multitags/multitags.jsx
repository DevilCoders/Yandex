import React from 'react';
import PropTypes from 'prop-types';

import {antiadbUrl} from 'app/lib/url';
import {ACTIONS} from 'app/enums/actions';

import MultiTagsItem from './__item/multitags__item';

export default class MultiTags extends React.Component {
    render() {
        return (
            <div>
                {this.props.tagItems.map(alertItem => {
                        return (
                            <MultiTagsItem
                                key={alertItem.name}
                                tagName={alertItem.name}
                                tagState={alertItem.state}
                                outdated={alertItem.outdated}
                                transitionTime={alertItem.transition_time}
                                tagUrl={antiadbUrl(`/service/${this.props.id}/${ACTIONS.HEALTH}`)} />
                        );
                })}

                {(this.props.otherItems.length > 0) ? (
                    <MultiTagsItem
                        tagName={'+' + this.props.otherItems.length.toString()}
                        tagState='other'
                        outdated={0}
                        transitionTime={0}
                        tagUrl={antiadbUrl(`/service/${this.props.id}/${ACTIONS.HEALTH}`)} />
                        ) : (
                            <div />
                        )
                    }
            </div>
        );
    }
}

MultiTags.propTypes = {
    id: PropTypes.string.isRequired,
    tagItems: PropTypes.array,
    otherItems: PropTypes.array
};

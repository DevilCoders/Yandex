import React from 'react';
import PropTypes from 'prop-types';

import './multitags__item.css';
import Button from 'lego-on-react/src/components/button/button.react';
import {Link} from 'react-router-dom';

const ANIMATED_PERIOD = 300;

export default class MultiTagsItem extends React.Component {
    chooseStyle(state, outdated, transitionTime) {
        let result = {};
        const currentTime = Math.floor((new Date()).getTime() / 1000);

        if (outdated) {
            result.outdated = true;
        } else {
            result.animated = (transitionTime > currentTime - ANIMATED_PERIOD);
        }
        result[state] = true;
        return result;
    }

    render() {
        return (
            <Link to={this.props.tagUrl}>
                <Button
                    size='xs'
                    theme='default'
                    mix={{
                        block: 'tag_burring',
                        mods: this.chooseStyle(this.props.tagState, this.props.outdated, this.props.transitionTime)
                    }}>{this.props.tagName}
                </Button>
            </Link>
        );
    }
}

MultiTagsItem.propTypes = {
    tagName: PropTypes.string.isRequired,
    tagState: PropTypes.string.isRequired,
    outdated: PropTypes.number.isRequired,
    transitionTime: PropTypes.number.isRequired,
    tagUrl: PropTypes.string
};

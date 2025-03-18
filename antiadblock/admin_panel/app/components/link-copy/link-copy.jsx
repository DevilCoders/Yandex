import React from 'react';
import PropTypes from 'prop-types';

import Link from 'lego-on-react/src/components/link/link.react';
import Hint from 'app/components/hint/hint';

import 'app/components/icon/_theme/icon_theme_link.css';

import copyToClipboard from 'app/lib/copy-to-clipboard';

export default class LinkCopy extends React.Component {
    constructor() {
        super();

        this.onCopyButtonClick = this.onCopyButtonClick.bind(this);
    }

    onCopyButtonClick(e) {
        e.stopPropagation();
        copyToClipboard(this.props.value);

        if (this.props.onClick) {
            this.props.onClick();
        }
    }

    render() {
        const to = this.props.to || 'right';
        const theme = this.props.theme || 'link';
        const {text, scope} = this.props;

        return (
            <Link
                theme='pseudo'
                onClick={this.onCopyButtonClick}>
                {this.props.description}
                {!this.props.disableHint &&
                    <Hint
                        text={text}
                        to={to}
                        theme={theme}
                        scope={scope}
                        timeout />}
            </Link>
        );
    }
}

LinkCopy.propTypes = {
    value: PropTypes.string.isRequired,
    text: PropTypes.string,
    to: PropTypes.string,
    scope: PropTypes.object,
    theme: PropTypes.string,
    description: PropTypes.string,
    disableHint: PropTypes.bool,
    onClick: PropTypes.func
};

LinkCopy.defaultProps = {
    description: ''
};

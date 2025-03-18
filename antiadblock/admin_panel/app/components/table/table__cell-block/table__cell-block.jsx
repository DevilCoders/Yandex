import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Popup from 'lego-on-react/src/components/popup/popup.react';
import Link from 'lego-on-react/src/components/link/link.react';
import Hint from 'app/components/hint/hint';

import copyToClipboard from 'app/lib/copy-to-clipboard';
import i18n from 'app/lib/i18n';

import './table__cell-block.css';

class TableCellBlock extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            isVisible: false
        };
        this._anchor = null;

        this.onChangeVisibilityPopup = this.onChangeVisibilityPopup.bind(this);
        this.onCopyButtonClick = this.onCopyButtonClick.bind(this);
        this.onClosePopup = this.onClosePopup.bind(this);
        this.setAnchorRef = this.setAnchorRef.bind(this);
    }

    onChangeVisibilityPopup() {
        this.setState(state => ({
            isVisible: !state.isVisible
        }));
    }

    onClosePopup() {
        this.setState({
            isVisible: false
        });
    }

    setAnchorRef(ref) {
        this._anchor = ref;
    }

    onCopyButtonClick(e) {
        e.stopPropagation();
        copyToClipboard(this.props.data);
    }

    render() {
        const {
            isVisible
        } = this.state;
        const {
            popup,
            data
        } = this.props;

        return (
            <Bem
                block='table-cell-block'>
                <Bem
                    block='table-cell-block'
                    elem='data'
                    mods={{
                        popup: popup
                    }}
                    tagRef={this.setAnchorRef}
                    onClick={popup ? this.onChangeVisibilityPopup : () => {}}>
                    {data}
                </Bem>
                {isVisible ?
                    <Popup
                        mix={{
                            block: 'test-aaa'
                        }}
                        hasTail
                        anchor={this._anchor}
                        directions={['left-center', 'left-bottom', 'left-top', 'right-center', 'right-bottom', 'right-top']}
                        autoclosable
                        onOutsideClick={this.onClosePopup}
                        onClose={this.onClosePopup}
                        visible={isVisible}
                        theme='normal'>
                        <Bem
                            block='table-cell-block'
                            elem='popup-container'>
                            <Bem
                                block='table-cell-block'
                                elem='popup-data'>
                                {data}
                            </Bem>
                            <Link
                                mix={{
                                    block: 'table-cell-block',
                                    elem: 'copy-link'
                                }}
                                theme='pseudo'
                                onClick={this.onCopyButtonClick}>
                                {i18n('common', 'copy')}
                                <Hint
                                    text={i18n('service-page', 'text-was-copied')}
                                    to={['top', 'bottom']}
                                    theme='copy-input'
                                    timeout />
                            </Link>
                        </Bem>

                    </Popup> : null}
            </Bem>
        );
    }
}

TableCellBlock.propTypes = {
    popup: PropTypes.bool,
    data: PropTypes.any
};

export default TableCellBlock;

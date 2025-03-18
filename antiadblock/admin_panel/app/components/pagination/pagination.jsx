import React, {Component} from 'react';
import PropTypes from 'prop-types';

import paginator from 'app/lib/paginator';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';

export default class Pagination extends Component {
    handleClick(pageNumber) {
        return e => {
            e.preventDefault();
            this.props.onChange(pageNumber);
        };
    }

    renderPages(paginationInfo) {
        let pages = [];

        for (let i = paginationInfo.firstPage; i <= paginationInfo.lastPage; i++) {
            pages.push(<Button
                key={`page-${i}`}
                checked={i === this.props.activePage}
                theme='normal'
                size='m'
                pin='brick-brick'
                text={`${i}`}
                onClick={this.handleClick(i)} />);
        }
        return pages;
    }

    render() {
        const {
            itemsCountPerPage,
            pageRangeDisplayed,
            activePage,
            totalItemsCount
        } = this.props;

        const paginationInfo = paginator(
            itemsCountPerPage,
            pageRangeDisplayed,
            totalItemsCount,
            activePage);

        return (
            <Bem
                key='pagination'
                block='pagination'>
                <Button
                    key='page-first'
                    theme='normal'
                    size='m'
                    pin='round-clear'
                    text='«'
                    onClick={this.handleClick(paginationInfo.firstPage)}
                    disabled={!paginationInfo.hasPreviousPage} />
                <Button
                    key='page-prev'
                    theme='normal'
                    size='m'
                    pin='brick-clear'
                    text='⟨'
                    onClick={this.handleClick(paginationInfo.previousPage)}
                    disabled={!paginationInfo.hasPreviousPage} />
                {this.renderPages(paginationInfo)}
                <Button
                    key='page-next'
                    theme='normal'
                    size='m'
                    pin='brick-clear'
                    text='⟩'
                    onClick={this.handleClick(paginationInfo.nextPage)}
                    disabled={!paginationInfo.hasNextPage} />
                <Button
                    key='page-last'
                    theme='normal'
                    size='m'
                    pin='brick-round'
                    text='»'
                    onClick={this.handleClick(paginationInfo.lastPage)}
                    disabled={paginationInfo.currentPage === paginationInfo.totalPages} />
            </Bem>
        );
    }
}

Pagination.propTypes = {
    totalItemsCount: PropTypes.number.isRequired,
    onChange: PropTypes.func,
    activePage: PropTypes.number,
    itemsCountPerPage: PropTypes.number,
    pageRangeDisplayed: PropTypes.number
};

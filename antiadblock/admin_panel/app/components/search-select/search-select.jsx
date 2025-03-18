import React from 'react';
import PropTypes from 'prop-types';

import Icon from 'lego-on-react/src/components/icon/icon.react';
import Select from 'lego-on-react/src/components/select/select.react';
import Bem from 'app/components/bem/bem';
import TextInput from 'lego-on-react/src/components/textinput/textinput.react';

import i18n from '../../lib/i18n';
import {KEYS} from '../../enums/keys';

import './search-select.css';
import 'app/components/icon/_theme/icon_theme_search.css';
import levenshtein from '../../lib/levenshtein';

const MAX_DIFF_DISTANCE_DEFAULT = 3;

export default class SearchSelect extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            filterText: '',
            selectedItem: 0
        };

        this._refs = {};
        this.select = null;
        this.setRef = this.setRef.bind(this);
        this.stopEvent = this.stopEvent.bind(this);

        this.onChangeFilter = this.onChangeFilter.bind(this);
        this.changeFocusSelectItem = this.changeFocusSelectItem.bind(this);
        this.renderSelectItems = this.renderSelectItems.bind(this);
        this.upDownAction = this.upDownAction.bind(this);
        this.eventClickSelect = this.eventClickSelect.bind(this);
        this.eventKeyDownInput = this.eventKeyDownInput.bind(this);
        this.onChangeSelect = this.onChangeSelect.bind(this);
    }

    componentDidMount() {
        // ищем select со списком сервисов, чтобы на него повесить event Click
        // для того чтобы валидно фокусить инпут внутри
        this.select = document.getElementsByClassName('search-select__select')[0];

        // останавливаем прокидывание блюра после фокуса инпута
        if (this._refs.select) {
            this._refs.select._onButtonBlur = this.stopEvent;
        }

        // через refs нельзя отловить клик, поэтому так
        if (this.select) {
            this.select.addEventListener('click', this.eventClickSelect);
        }
    }

    setRef(name) {
        return value => {
            this._refs[name] = value;
        };
    }

    stopEvent(e) {
        e.preventDefault();
        e.stopPropagation();
    }

    onChangeFilter(value) {
        this.setState({
            filterText: value,
            selectedItem: 0
        });

        this.changeFocusSelectItem(0);
    }

    upDownAction(selectedItem, e) {
        this.setState({
            selectedItem: selectedItem
        });
        this.changeFocusSelectItem(selectedItem);
        this.stopEvent(e);
    }

    onChangeSelect(val) {
        const data = Array.isArray(val) ? val[0] : val;

        this.props.onChangeSelect(data);
    }

    eventKeyDownInput(e) {
        let {
            selectedItem
        } = this.state;

        switch (e.keyCode) {
            case KEYS.ENTER:
                // в items все элементы select
                if (this._refs.select && this._refs.select._menu.items.length) {
                    this.onChangeSelect(this._refs.select._menu.items[selectedItem].props.val);
                }
                this.stopEvent(e);
                break;
            case KEYS.UP:
                selectedItem = selectedItem - 1 < 0 ? this._refs.select && this._refs.select._menu.items.length - 1 : selectedItem - 1;
                this.upDownAction(selectedItem, e);
                break;
            case KEYS.DOWN:
                selectedItem = selectedItem + 1 > this._refs.select && this._refs.select._menu.items.length - 1 ? 0 : selectedItem + 1;
                this.upDownAction(selectedItem, e);
                break;
            default:
                break;
        }
    }

    changeFocusSelectItem(index) {
        if (this._refs.select && this._refs.select._menu && this._refs.select._menu.items && this._refs.select._menu.items.length) {
            // При перерисовки select надо чтобы событие было последним в очереди
            setTimeout(() => {
                this._refs.select._menu.items[index].onFocus();
            }, 0);
        }
    }

    eventClickSelect() {
        // здесь проверяем что не найдено, т.к. класс проставляется после этого события уже в лего
        const selectIsOpened = this.select && this.select.classList.value.indexOf('select2_opened_yes') === -1;

        if (selectIsOpened) {
            // вешаем обработчик доп. нажатий на инпут
            this._refs.input.onKeyDown = this.eventKeyDownInput;
            // событие должно бывать вызвано после события раскрытия селекта
            setTimeout(() => {
                this._refs.input.focus();
            }, 0);
        }
    }

    clearSentence(str) {
        return str.replace(/[,.?_-]/g, ' ').replace(/\s+/g, ' ').trim();
    }

    getMinDistance(str, filter) {
        const subStr = this.clearSentence(str).split(' ');
        const distanceStr = subStr.map(str => {
            return levenshtein(str, filter);
        });
        return Math.min(...distanceStr);
    }

    renderTextInput() {
        return (
            <TextInput
                key='input'
                theme='normal'
                tone='grey'
                view='default'
                size='m'
                mix={{
                    block: 'search-select',
                    elem: 'input'
                }}
                ref={this.setRef('input')}
                text={this.state.filterText}
                placeholder={i18n('header', 'search-services')}
                iconLeft={
                    <Icon
                        size='s'
                        mix={{
                            block: 'icon',
                            mods: {
                                theme: 'search'
                            }
                        }} />
                }
                onChange={this.onChangeFilter} />
        );
    }

    renderDefaultSelectItems(items) {
        return (
            items.map(item => (
                <Select.Item
                    key={item}
                    val={item}>
                    {item}
                </Select.Item>
            ))
        );
    }

    getMaxDiffDistance() {
        return Number.isFinite(this.props.maxDiffDistance) || MAX_DIFF_DISTANCE_DEFAULT;
    }

    filteringData() {
        const defaultFilter = item => item.indexOf(this.state.filterText) !== -1;
        const levenshteinFilter = item => Math.min(
            this.getMinDistance(item.toString().toLowerCase(), this.state.filterText.toLowerCase()),
        ) < this.getMaxDiffDistance();

        const items = this.props.selectItems.filter(defaultFilter);

        return items.length === 0 ? this.props.selectItems.filter(levenshteinFilter) : items;
    }

    renderSelectItems() {
        const items = this.filteringData();

        return [
            this.props.customSelectItems || this.renderDefaultSelectItems(items),
            (this.props.customSelectItems || items).length === 0 && (
                <Select.Item
                    disabled>
                    {i18n('header', 'not-found-service')}
                </Select.Item>
            )
        ];
    }

    render() {
        return (
            <Bem block='search-select'>
                <Select
                    theme='normal'
                    view='default'
                    tone='grey'
                    size='m'
                    width='max'
                    mix={{
                        block: 'search-select',
                        elem: 'select'
                    }}
                    ref={this.setRef('select')}
                    type='radio'
                    onChange={this.onChangeSelect}
                    placeholder={this.props.selectedItemText || i18n('header', 'select-service')}
                    val={this.props.selectedItemText}>
                    {this.renderTextInput()}
                    {this.renderSelectItems()}
                </Select>
            </Bem>
        );
    }
}

SearchSelect.propTypes = {
    customSelectItems: PropTypes.arrayOf(PropTypes.node),
    filter: PropTypes.func,
    selectItems: PropTypes.array,
    selectedItemText: PropTypes.string,
    onChangeSelect: PropTypes.func,
    maxDiffDistance: PropTypes.number
};

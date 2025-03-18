import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {fetchServices} from 'app/actions/services';
import {getServices} from 'app/reducers/index';

import Icon from 'lego-on-react/src/components/icon/icon.react';
import Select from 'lego-on-react/src/components/select/select.react';
import Button from 'lego-on-react/src/components/button/button.react';
import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import TextInput from 'lego-on-react/src/components/textinput/textinput.react';

import {antiadbUrl} from 'app/lib/url';
import i18n from 'app/lib/i18n';
import {getUser} from 'app/lib/user';
import {getPermissions} from 'app/lib/permissions';
import levenshtein from 'app/lib/levenshtein';

import StatusCircle from 'app/components/status-circle/status-circle';

import {serviceType} from 'app/types';

import './services-list.css';
import 'app/components/icon/_theme/icon_theme_search.css';
import {STATUS} from 'app/enums/service';

import {KEYS} from 'app/enums/keys';

const MAX_DIFF_DISTANCE = 3;

class ServicesList extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            filter: '',
            selectedItem: 0
        };

        this._refs = {};

        this.onChangeFilter = this.onChangeFilter.bind(this);
        this.onChangeUrl = this.onChangeUrl.bind(this);
        this.eventClickSelect = this.eventClickSelect.bind(this);
        this.eventKeyDownInput = this.eventKeyDownInput.bind(this);
        this.setRef = this.setRef.bind(this);
        this.stopEvent = this.stopEvent.bind(this);
    }

    onChangeUrl(value) {
        if (value) {
            this.props.onChange(`/service/${value}`);
        }
    }

    onChangeFilter(value) {
        this.setState({
            filter: value,
            selectedItem: 0
        });

        this.changeFocusSelectItem(0);
    }

    changeFocusSelectItem(index) {
        if (this._refs.select._menu.items.length) {
            // При перерисовки select надо чтобы событие было последним в очереди
            setTimeout(() => {
                this._refs.select._menu.items[index].onFocus();
            }, 0);
        }
    }

    setRef(name) {
        return value => {
            this._refs[name] = value;
        };
    }

    eventKeyDownInput(e) {
        let {
            selectedItem
        } = this.state;

        switch (e.keyCode) {
            case KEYS.ENTER:
                // в items все элементы select
                if (this._refs.select._menu.items.length) {
                    this.onChangeUrl(this._refs.select._menu.items[selectedItem].props.val);
                }
                e.preventDefault();
                e.stopPropagation();
                break;
            case KEYS.UP:
                selectedItem = selectedItem - 1 < 0 ? this._refs.select._menu.items.length - 1 : selectedItem - 1;
                this.setState({
                    selectedItem: selectedItem
                });
                this.changeFocusSelectItem(selectedItem);
                this.stopEvent(e);
                break;
            case KEYS.DOWN:
                selectedItem = selectedItem + 1 > this._refs.select._menu.items.length - 1 ? 0 : selectedItem + 1;
                this.setState({
                    selectedItem: selectedItem
                });
                this.changeFocusSelectItem(selectedItem);
                this.stopEvent(e);
                break;
            default:
                break;
        }
    }

    eventClickSelect() {
        const select = document.getElementsByClassName('services-list__select')[0];
        // здесь проверяем что не найдено, т.к. класс проставляется после этого события уже в лего
        const selectIsOpened = select.classList.value.indexOf('select2_opened_yes') === -1;

        if (selectIsOpened) {
            // вешаем обработчик доп. нажатий на инпут
            this._refs.input.onKeyDown = this.eventKeyDownInput;
            // событие должно бывать вызвано после события раскрытия селекта
            setTimeout(() => {
                this._refs.input.focus();
            }, 0);
        }
    }

    componentDidMount() {
        // ищем select со списком сервисов, чтобы на него повесить event Click
        // для того чтобы валидно фокусить инпут внутри
        const select = document.getElementsByClassName('services-list__select')[0];

        // останавливаем прокидывание блюра после фокуса инпута
        this._refs.select._onButtonBlur = this.stopEvent;
        // через refs нельзя отловить клик, поэтому так
        select.addEventListener('click', this.eventClickSelect);

        this.props.fetchServices();
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

    stopEvent(e) {
        e.preventDefault();
        e.stopPropagation();
    }

    render() {
        const user = getUser();
        const permissions = getPermissions();
        const filterValue = this.state.filter;
        const {
            items,
            serviceId,
            loaded
        } = this.props;

        const filter = item => ((item.status === STATUS.ACTIVE || filterValue.length > 0) || item.id === serviceId) &&
            (item.name.indexOf(filterValue) !== -1 || item.domain.indexOf(filterValue) !== -1);
        let activeItems = items.filter(filter);

        if (activeItems.length === 0) {
            activeItems = this.props.items.filter(item => (
                Math.min(
                    this.getMinDistance(item.name.toLowerCase(), filterValue.toLowerCase()),
                    this.getMinDistance(item.domain.toLowerCase(), filterValue.toLowerCase())
                ) < MAX_DIFF_DISTANCE
            ));
        }

        return (
            <Bem block='services-list'>
                {!loaded && <Preloader />}
                <Select
                    theme='normal'
                    view='default'
                    tone='grey'
                    size='m'
                    width='max'
                    mix={{
                        block: 'services-list',
                        elem: 'select'
                    }}
                    ref={this.setRef('select')}
                    type='radio'
                    onChange={this.onChangeUrl}
                    placeholder={serviceId || i18n('header', 'select-service')}
                    val={serviceId}>
                    <TextInput
                        key='input'
                        theme='normal'
                        tone='grey'
                        view='default'
                        size='m'
                        mix={{
                            block: 'services-list',
                            elem: 'search-input'
                        }}
                        ref={this.setRef('input')}
                        text={filterValue}
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
                    {activeItems.map(item => (
                        <Select.Item
                            icon={(
                                <Icon
                                    mix={{
                                        block: 'services-list',
                                        elem: 'select-icon'
                                    }}>
                                    <StatusCircle status={item.status} />
                                </Icon>
                            )}
                            key={item.id}
                            val={item.id}>
                            {item.name}
                        </Select.Item>
                    ))}
                    {activeItems.length === 0 && (
                        <Select.Item disabled>{i18n('header', 'not-found-service')}</Select.Item>
                    )}
                </Select>

                {user.hasPermission(permissions.SERVICE_CREATE) && (
                    <Button
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='m'
                        type='link'
                        url={antiadbUrl('/service')}
                        mix={{
                            block: 'services-list',
                            elem: 'add'
                        }}>
                        +
                    </Button>
                )}
            </Bem>
        );
    }
}

ServicesList.propTypes = {
    items: PropTypes.arrayOf(serviceType).isRequired,
    serviceId: PropTypes.string,
    fetchServices: PropTypes.func.isRequired,
    loaded: PropTypes.bool.isRequired,
    onChange: PropTypes.func.isRequired
};

export default connect(state => {
    return {
        ...getServices(state)
    };
}, dispatch => {
    return {
        fetchServices: () => {
            dispatch(fetchServices());
        }
    };
})(ServicesList);

import React from "react";
import {Link, NavLink} from "react-router-dom";
import {Textinput} from "@yandex-lego/components/Textinput/desktop/bundle";
import {Select} from "@yandex-lego/components/Select/desktop/bundle";
import {Text} from "@yandex-lego/components/Text/desktop/bundle";
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSearch } from '@fortawesome/free-solid-svg-icons'

import './Header.css'
import {Spin} from "@yandex-lego/components/Spin/desktop/bundle";
import {useSelector} from "react-redux";
import {IHeaderState} from "../../reducers/headerReducer";
import {IState} from "../../reducers";

const Header: React.FC = () => {
    const headerState = useSelector<IState, IHeaderState>(state => state.header);
    return(
        <div className="header" >
            <Link to="/" className="link">
                <img
                    src="https://yastatic.net/q/logoaas/v1/MDB UI.svg"
                    alt="MDB UI"
                    className="logo"
                />
            </Link>
            <div className="menu">
                <NavLink to="/masters" className="link" activeClassName="active-link">Masters</NavLink>
                <NavLink to="/minions" className="link" activeClassName="active-link">Minions</NavLink>
                <NavLink to="/shipments" className="link" activeClassName="active-link">Shipments</NavLink>
                <NavLink to="/jobresults" className="link" activeClassName="active-link">Job Results</NavLink>
            </div>

            <div className="right-side">
                <Spin view="default" size="m" progress={headerState.loadingCount > 0}/>
                <Text typography="body-short-m" color="alert" overflow="ellipsis">
                    {headerState.lastError && headerState.lastError.message ? headerState.lastError.message : ""}
                </Text>
                <Textinput
                    size="m"
                    view="default"
                    placeholder="fqdn, shipment, jid"
                    iconLeft={<FontAwesomeIcon icon={faSearch} listItem style={{ marginTop: 10}} />}
                    style={{width: "200px"}}
                />
                <Select
                    size="m"
                    view="default"
                    placeholder="Env"
                    options={[
                        { value: '1', content: 'porto-prod' },
                        { value: '2', content: 'porto-test' },
                        { value: '3', content: 'compute-prod' },
                        { value: '4', content: 'preprod' },
                    ]}
                />
                <img src="//center.yandex-team.ru/api/v1/user/annkpx/avatar/50.jpg" alt="avatar"/>
            </div>
        </div>
    );
};

export default Header;

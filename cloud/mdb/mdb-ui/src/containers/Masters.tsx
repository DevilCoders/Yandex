import React, {useEffect} from 'react';

import {Textinput} from "@yandex-lego/components/Textinput/desktop/bundle";
import {Select} from "@yandex-lego/components/Select/desktop/bundle";
import {Button} from "@yandex-lego/components/Button/desktop/bundle";

import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSearch } from '@fortawesome/free-solid-svg-icons'

import {useDispatch, useSelector} from "react-redux";
import {IState} from "../reducers";
import {IMastersState} from "../reducers/mastersReducer";
import {mastersList} from "../actions/mastersAction";
import MasterList from "../components/MasterList";
import Sidebar from "../components/Sidebar";


const Masters : React.FC = () => {
    const mastersState = useSelector<IState, IMastersState>(state => state.masters);
    const dispatch = useDispatch();
    useEffect(()=>{
        dispatch(mastersList());
    }, []);  // eslint-disable-line
    return (
        <>
            <Sidebar title="Filters">
                <Select
                    size="s"
                    view="default"
                    placeholder="Deploy Group"
                    options={[
                        { value: '1', content: 'porto-prod' },
                        { value: '42', content: 'isiv' },
                    ]}
                    width="max"
                />
                <Textinput
                    size="s"
                    view="default"
                    placeholder="fqdn"
                    iconLeft={<FontAwesomeIcon icon={faSearch} listItem style={{ marginTop: 7}} />}
                />
                <br/><br/>
                <Button view="action" size="s" width="max">Apply</Button>
            </Sidebar>
            <div className="content">
                <h1>Masters</h1>
                <MasterList masters={mastersState.masters || []}/>
            </div>
        </>
    );
};

export default Masters;

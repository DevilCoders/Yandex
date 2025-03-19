import React, {useEffect, useState} from 'react';

import {Textinput} from "@yandex-lego/components/Textinput/desktop/bundle";
import {Select} from "@yandex-lego/components/Select/desktop/bundle";
import {Button} from "@yandex-lego/components/Button/desktop/bundle";
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSearch } from '@fortawesome/free-solid-svg-icons'
import {Tumbler} from "@yandex-lego/components/Tumbler/desktop/bundle";

import MinionList from "../components/MinionList";
import Sidebar from "../components/Sidebar";
import {useDispatch, useSelector} from "react-redux";
import {minionsList} from "../actions/minionsAction";
import {IState} from "../reducers";
import {IMinionsState} from "../reducers/minionsReducer";


const Minions : React.FC = () => {
    const dispatch = useDispatch();
    const [showErrorsOnly, setShowErrorsOnly] = useState(false);
    const minionsState = useSelector<IState, IMinionsState>(state => state.minions);
    useEffect(() => {
        dispatch(minionsList());
    }, [])  // eslint-disable-line
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
                    placeholder="fqdn, master"
                    iconLeft={<FontAwesomeIcon icon={faSearch} listItem style={{ marginTop: 7}} />}
                />
                <Tumbler
                    view="default"
                    size="m"
                    checked={showErrorsOnly}
                    onChange={() => {setShowErrorsOnly(!showErrorsOnly);}}
                    labelAfter="Last job failed"
                />
                <br/><br/>
                <Button view="action" size="s" width="max" onClick={()=>dispatch(minionsList())}>Apply</Button>
            </Sidebar>
            <div className="content">
                <h1>Minions</h1>
                <MinionList minions={minionsState.minions || []}/>
            </div>
        </>
    );
};

export default Minions;

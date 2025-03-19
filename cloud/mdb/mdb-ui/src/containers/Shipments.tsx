import React, {useEffect, useState} from 'react';
import {useDispatch, useSelector} from "react-redux";

import {Textinput} from "@yandex-lego/components/Textinput/desktop/bundle";
import {Select} from "@yandex-lego/components/Select/desktop/bundle";
import {Button} from "@yandex-lego/components/Button/desktop/bundle";
import {Tumbler} from "@yandex-lego/components/Tumbler/desktop/bundle";
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSearch } from '@fortawesome/free-solid-svg-icons'

import ShipmentList from "../components/ShipmentList";
import {shipmentsList} from "../actions/shipmentsAction";

import {IState} from "../reducers";
import {IShipmentsState} from "../reducers/shipmentsReducer";
import Sidebar from "../components/Sidebar";

const Shipments : React.FC = () => {
    const shipmentState = useSelector<IState, IShipmentsState>(state => state.shipments);
    const [showErrorsOnly, setShowErrorsOnly] = useState(shipmentState.showErrorsOnly);
    const dispatch = useDispatch();
    useEffect(()=>{
        dispatch(shipmentsList(shipmentState.showErrorsOnly))
    },[])  // eslint-disable-line

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
                <Textinput
                    size="s"
                    view="default"
                    placeholder="state"
                    iconLeft={<FontAwesomeIcon icon={faSearch} listItem style={{ marginTop: 7}} />}
                />

                <Button pin="circle-clear" view="default" size="m">Progress</Button>
                <Button pin="clear-clear" view="default" size="m">Done</Button>
                <Button pin="clear-clear" view="default" size="m">Timeout</Button>
                <Button pin="clear-circle" view="default" size="m">Failed</Button>

                <Tumbler
                    view="default"
                    size="m"
                    checked={showErrorsOnly}
                    onChange={() => {setShowErrorsOnly(!showErrorsOnly); dispatch(shipmentsList(!showErrorsOnly))}}
                    labelAfter="Show errors only"
                /><br />

                <br/><br/>
                <Button view="action" size="s" width="max">Apply</Button>
            </Sidebar>

            <div className="content">
                <h1>Shipments</h1>
                <ShipmentList shipments={shipmentState.shipments || []}/>
            </div>
        </>
    );
};

export default Shipments;

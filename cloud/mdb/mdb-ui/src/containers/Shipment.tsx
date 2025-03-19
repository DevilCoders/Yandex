import React, {useEffect, useState} from "react";
import {useDispatch, useSelector} from "react-redux";
import {useParams} from 'react-router-dom';
import {IState} from "../reducers";
import {IShipmentState} from "../reducers/shipmentReducer";
import {shipmentLoad} from "../actions/shipmentAction";
import Sidebar from "../components/Sidebar";
import {Text} from "@yandex-lego/components/Text/desktop/bundle";
import {Divider} from "@yandex-lego/components/Divider/desktop";
import JSONPretty from "react-json-pretty";
import {Tumbler} from "@yandex-lego/components/Tumbler/desktop/bundle";
import {shipmentsList} from "../actions/shipmentsAction";
import {IJobResult} from "../models";
import {JobResultStatus} from "../models/deployapi";

const JSONPrettyAcai = require("react-json-pretty/dist/1337");

// TODO Separate to different components
const Shipment : React.FC = () => {
    const {id} = useParams();
    const dispatch = useDispatch();
    useEffect(()=>{
        dispatch(shipmentLoad(id))
    },[])  // eslint-disable-line
    const shipmentState = useSelector<IState, IShipmentState>(state => state.shipment);
    const [showErrorsOnly, setShowErrorsOnly] = useState(false);

    return (
        <>
            <Sidebar title="Shipment & Jobs">
                <Text as="div" typography="display-s">Shipment</Text>
                <Text as="span" typography="subheader-m">{shipmentState.shipment?.shipment.id} ({shipmentState.shipment?.shipment.status})</Text>
                <ul style={{paddingLeft: "10px"}}>
                    {shipmentState.shipment?.shipment.fqdns?.map(f => <Text as="li" key={f} typography="subheader-m">{f}</Text>)}
                </ul>
                <br/>
                <Text typography="display-m">Jobs</Text>
                {shipmentState.shipment?.jobs.map(j => {
                    return (
                        <div key={j.extId}>
                            <Divider color="blue"/>
                            <Text as="div" typography="headline-m">{j.extId}</Text>
                            <Text as="div" typography="subheader-m">{j.status}</Text>
                        </div>
                    );
                })}
                <Tumbler
                    view="default"
                    size="m"
                    checked={showErrorsOnly}
                    onChange={() => {setShowErrorsOnly(!showErrorsOnly); dispatch(shipmentsList(!showErrorsOnly))}}
                    labelAfter="Show errors only"
                />

            </Sidebar>
            <div className="content">
                <h1>Job results</h1>
                {shipmentState.shipment?.jobResults?.filter(jr => jr.status !== JobResultStatus.success || !showErrorsOnly).map(jr => {
                    const result: IJobResult = JSON.parse(atob(jr.result||""));
                    let states = Object.values(result.return).filter((s:any)=> !s["result"] || !showErrorsOnly);
                    return (
                        <div key={jr.id}>
                            <Divider color="green"/>
                            <Text as="div" typography="headline-m">{jr.id} ({jr.status})</Text>
                            <Text as="div" typography="headline-m">{result.fun}</Text>
                            <JSONPretty theme={JSONPrettyAcai} data={
                                result.fun !== "state.highstate" ? result.return : states
                            }/>
                        </div>
                    );
                })}
            </div>
        </>
    );
}

export default Shipment;

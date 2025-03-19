import React from "react";
import ShipmentItem from "../../components/ShipmentList/ShipmentItem";

import './ShipmentItem.css'
import {ShipmentResp} from "../../models/deployapi";

interface IShipmentListProps {
    shipments: ShipmentResp[];
}

const ShipmentList: React.FC<IShipmentListProps> = ({shipments}) => {
    const shipmentItems = shipments.map((s) => <ShipmentItem key={s.id} shipment={s} />);
    return (
        <table className="table">
            <thead>
            <tr>
                <th/>
                <th>ID</th>
                <th>Hosts</th>
                <th>Created</th>
                <th>Status</th>
                <th>Commands</th>
                <th>Job Results</th>
            </tr>
            </thead>
            <tbody>
            {shipmentItems}
            </tbody>
            <tfoot>
                <th></th><th></th><th></th><th></th><th></th><th></th><th></th><th></th>
            </tfoot>
        </table>
    );
}

export default ShipmentList;

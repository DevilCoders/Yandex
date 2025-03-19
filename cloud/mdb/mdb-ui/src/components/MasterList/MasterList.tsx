import React from "react";
import MasterItem from "./MasterItem";
import {MasterResp,} from "../../models/deployapi";

interface IMasterListProps {
    masters: MasterResp[]
}

const MasterList : React.FC<IMasterListProps> = ({masters}) => {
    const mastersItems = masters.slice()
        .sort((a, b) => {
            let a_fqdn = a.fqdn || "";
            let b_fqdn = b.fqdn || "";
            if (a_fqdn > b_fqdn) {
                return 1;
            } else if (a_fqdn < b_fqdn) {
                return -1;
            }
            return 0;
        })
        .map((m) => <MasterItem key={m.fqdn} master={m} />);
    return (
        <table className="table">
            <thead>
            <tr>
                <th>Master</th>
                <th>Group Name</th>
                <th>Created At</th>
                <th>Status</th>
            </tr>
            </thead>
            <tbody>
            {mastersItems}
            </tbody>
            <tfoot>
            <tr>
                <th/><th/><th/><th/>
            </tr>
            </tfoot>
        </table>
    );
};

export default MasterList;

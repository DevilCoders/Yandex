import React from "react";

import {Link} from "@yandex-lego/components/Link/desktop/bundle";
import {Text} from "@yandex-lego/components/Text/desktop/bundle";
import {MinionResp} from "../../models/deployapi";


interface IMinionItemProps {
    minion: MinionResp
}
const MinionItem : React.FC<IMinionItemProps> = ({minion})=> {
    return (
        <tr>
            <td><Link view="default" href="/minion/21129/">{minion.fqdn}</Link></td>
            <td><Link view="default" href="/master/39/">{minion.master}</Link></td>
            <td><Text as="span" typography="body-short-m">{new Date((minion.createdAt||0) * 1000).toLocaleString()}</Text></td>
            <td><Link view="default" href="/shipment/3/" className="Text_color_alert">{minion.registered}</Link></td>
            <td><Link view="default" href="/jobresult/2/" className="Text_color_alert">{minion.deleted}</Link></td>
        </tr>

    );
}


interface IMinionListProps {
    minions: MinionResp[]
}

const MinionList : React.FC<IMinionListProps> = ({minions}) => {
    return (
        <div>
            <table className="table">
                <thead>
                <tr>
                    <th>Minion</th>
                    <th>Master</th>
                    <th>Created At</th>
                    <th>Registerd</th>
                    <th>Deleted</th>
                </tr>
                </thead>
                <tbody>
                    {minions.map(m => <MinionItem minion={m}/>)}
                </tbody>
                <tfoot>
                <tr>
                    <th/><th/><th/><th/><th/><th/>
                </tr>
                </tfoot>
            </table>

            <ul className="paginator">
                <li><span>1</span></li>
                <li><a href="/minions/?page=2">2</a></li>
            </ul>
        </div>
    );
};

export default MinionList;

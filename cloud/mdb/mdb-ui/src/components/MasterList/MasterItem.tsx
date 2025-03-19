import {MasterResp} from "../../models/deployapi";
import React from "react";
import {Link} from "@yandex-lego/components/Link/desktop/bundle";
import {Text, TextColorValue} from "@yandex-lego/components/Text/desktop/bundle";
import {TooltipStateful} from "@yandex-lego/components/Tooltip/desktop/bundle";

interface IMasterItemProps {
    master: MasterResp
}

const MasterItem: React.FC<IMasterItemProps> = ({master}) => {
    return (
        <tr>
            <td><Link view="default" href="/master/1/" theme="black">{master.fqdn}</Link></td>
            <td><Link view="default" href="/masters/group=1" theme="ghost">{master.group}</Link></td>
            <td><Text as="span" typography="body-short-m">{new Date((master.createdAt||0) * 1000).toLocaleString()}</Text></td>
            <td>
                <Text typography="body-short-m" color={master.isOpen ? 'success': 'alert' as TextColorValue}>
                    {master.isOpen ? 'Open': 'Closed'}
                </Text>,&nbsp;
                <TooltipStateful view="default" size="m" content={new Date((master.aliveCheckAt||0) * 1000).toLocaleString()}>
                    <Text typography="body-short-m" color={master.isAlive ? 'success': 'alert' as TextColorValue}>
                        {master.isAlive ? 'Alive': 'Unavailable'}
                    </Text>
                </TooltipStateful>
            </td>
        </tr>
    );
}

export default MasterItem;

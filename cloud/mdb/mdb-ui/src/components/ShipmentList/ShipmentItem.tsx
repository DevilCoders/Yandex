import React from "react";

import {CommandDef, ShipmentResp, ShipmentStatus} from "../../models/deployapi";
import {TooltipStateful} from "@yandex-lego/components/Tooltip/desktop/bundle";
import {Text} from "@yandex-lego/components/Text/desktop/bundle";
import {Link} from "@yandex-lego/components/Link/desktop/bundle";
import { Badge } from '@yandex-lego/components/Badge'

import {cn} from "@bem-react/classname";

const si = cn('ShipmentItem');

interface IShipmentItemProps {
    shipment: ShipmentResp
}

const getIconTypeByStatus = (status: ShipmentStatus): string  => {
    switch (status) {
        case ShipmentStatus.done:
            return '\u2705';
        case ShipmentStatus.error:
            return '\u274C'
        case ShipmentStatus.inprogress:
            return '\u25B6'
        default:
            return "\u2753";
    }
};

const renderCommand = (command: CommandDef): JSX.Element => {
    return (
        <>
            <div  className={si('CommandType')}>
                {command.type}
            </div>
            <ul>{command.arguments?.map((a) => <li key={a}  className={si('CommandArguments')}>{a}</li>)}</ul>
        </>
    );
}

const renderCommands = (commands: CommandDef[]): JSX.Element => {
    return <ol  className={si('CommandList')}>{commands.map((c) => <li key={c.type}>{renderCommand(c)}</li>)}</ol>;
}

const renderHosts = (fqdns: string[]): JSX.Element => {
    return <ul>{fqdns.map((f) => <li key={f}>{f}</li>)}</ul>;
}

const ShipmentItem: React.FC<IShipmentItemProps> = ({shipment}) => {
    return (
        <tr className={si()}>
            <td>
                <TooltipStateful view="default" size="m" content={new Date((shipment.updatedAt||0) * 1000).toLocaleString()}>
                    <span>{getIconTypeByStatus(shipment.status || ShipmentStatus.unknown)}</span>
                </TooltipStateful>
            </td>
            <td>
                <Link view="default" href={"/shipments/" + shipment.id} theme="black">{shipment.id}</Link>
            </td>
            <td>
                {renderHosts(shipment.fqdns || [])}
            </td>
            <td>
                <Text as="span" typography="body-short-m">{new Date((shipment.createdAt||0) * 1000).toLocaleString()}</Text>
            </td>
            <td>{shipment.status}</td>
            <td className="shipment-commands">
                <Text
                    typography="body-short-m"
                    weight="light"
                    maxLines={6}
                    overflow="fade-horizontal"
                  >
                    {renderCommands(shipment.commands || [])}
                </Text>

            </td>
            <td>
                <Badge content="45" style={{ marginLeft: 5 }}>
                    <Link view="default" href="/jobresult/2/">123</Link>
                </Badge>
            </td>
        </tr>
    );
}

export default ShipmentItem;

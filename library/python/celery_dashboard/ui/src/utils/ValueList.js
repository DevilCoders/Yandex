import React from 'react';
import List from '@material-ui/core/List';
import ListItem from '@material-ui/core/ListItem';
import ListItemText from '@material-ui/core/ListItemText';


export function ValueList(value) {
    if (!value)
        return <List />

    return <List>
        {value.map(x => <ListItem>
            <ListItemText primary={x}/>
        </ListItem>)}
    </List>
}

export function ValueListFromDict(values) {
    if (!values)
        return <ListItem />

    return Object.entries(values).map(
        ([key, value]) => {
            return <ListItem>
                <ListItemText
                    primary={
                        value instanceof Object ? ValueListFromDict(value) : `${key}=${value}` }
                />
            </ListItem>
        })
}

export function ArgsKwargsToList(args, kwargs) {
    return <List>
        ARGS
        { ValueListFromDict(args) }
        KWARGS
        { ValueListFromDict(kwargs) }
    </List>
}

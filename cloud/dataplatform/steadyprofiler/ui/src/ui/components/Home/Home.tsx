import React, { useState, useEffect } from 'react';
import './Home.scss';
import { Header, Tabs, TabsDirection, Loader, Link, Timeline, TextInput, HelpTooltip, Button } from '@yandex-data-ui/common';
import { Table, withTableSorting } from '@yandex-data-ui/common';

export interface ServiceList {
    Services?: (ServicesEntity)[] | null;
}
export interface ServicesEntity {
    Name: string;
    Resources: Array<string>;
}
export interface ResourceEntity {
    Service: string;
    ProfileType: string;
    ResourceID: string;
    Count: number;
    MinTS: number;
    MaxTS: number;
    Links?: (string)[] | null;
}

export interface ProfileList {
    Service: string;
    ProfileType: string;
    ResourceID: string;
    Labels: Map<string, string>;
    TS: string;
    TSNano: number;
    Links: (LinksEntity)[];
}
export interface LinksEntity {
    URL: string;
    Label: string;
}


export default function Home() {
    const [services, setServices] = useState<Array<{ id: string, title: string, item: ServicesEntity}>>([]);
    const [activeService, setActiveService] = useState<{ id: string, title: string, item: ServicesEntity}>()
    useEffect(() => {
        fetch('/api/').then<ServiceList>(result => {
            return result.json()
        }).then(json => {
            if (!json.Services) {
                return
            }
            const tabs = json.Services.map(t => {
                return {
                    id: t.Name,
                    title: `${t.Name} with ${t.Resources.length} resources`,
                    item: t
                }
            })
            setServices(tabs);
            if (tabs.length > 0) {
                setActiveService(tabs[0])
            }
        });
    }, []);
    if (!activeService) {
        return <Loader size="l" className="active_tab__content_loader" />
    }
    return (
        <div>
            <Header
                logoText="Yandex GoProf"
                userInternal
            />
            <div className="home">
                <Tabs
                    direction={TabsDirection.Horizontal}
                    items={services}
                    activeTab={activeService.id}
                    onSelectTab={(tabId) => {
                        setActiveService(services.find(t => t.id == tabId))
                    }}
                />
                {activeService && activeService.item ? <ActiveTab tab={activeService} /> : <Loader />}
            </div>
        </div>
    );
}

const ActiveTab = ({ tab }: { tab: { id: string, title: string, item: ServicesEntity} }) => {
    if (!tab) {
        return <div />
    }
    const resourceIds = tab.item.Resources
    const from = new Date()
    from.setDate(from.getDate() - 1)
    const [timeline, setTimeline] = useState({
        from: from.getTime(),
        to: new Date().getTime()
    })
    const [filter, setFilter] = useState("")
    const [selectedType, setSelectedType] = useState("cpu")
    const [types, setTypes] = useState<Array<string>>([])
    const [resourceId, setResourceId] = useState<string>(resourceIds[0])
    const [profiles, setProfiles] = useState<Array<ProfileList>>([]);
    const [isLoad, setIsLoad] = useState(false)
    useEffect(() => {
        if (!resourceId) {
            return
        }
        setIsLoad(true)
        fetch(`/api/service/${tab.id}/type/${selectedType}/resource/${resourceId}/profiles?min_ts=${new Date(timeline.from).toISOString()}&max_ts=${new Date(timeline.to).toISOString()}`)
        .then<Array<ProfileList>>(result => {
            return result.json()
        }).then(json => {
            setProfiles(json)
        }).then(() => {
            return fetch(`/api/service/${tab.id}/types`)
        }).then<Array<string>>((result) => {
            return result.json()
        }).then((json) => {
            setTypes(json)
            setIsLoad(false)
        });
    }, [tab.id, resourceId, selectedType, timeline.from, timeline.to]);
    if (!resourceId) {
        return <div />
    }
    return <div>
        <Timeline from={timeline.from} to={timeline.to} onUpdate={({ from, to }) => setTimeline({ from, to })} />
        <div className="active_tab">
            <div className="active_tab__sidebar">
                <TextInput
                    size="xl"
                    placeholder="filter resource"
                    value={filter}
                    onUpdate={(val) => setFilter(val)}
                />
                <Tabs
                    direction={TabsDirection.Vertical}
                    items={resourceIds.filter(t => t.includes(filter)).map(t => { return { id: t, title: t } })}
                    activeTab={resourceId}
                    onSelectTab={tabId => setResourceId(tabId)}
                />
            </div>
            {isLoad ? <Loader size="l" className="active_tab__content_loader" /> :  <div className="active_tab__content">
                <Tabs
                    direction={TabsDirection.Horizontal}
                    activeTab={selectedType}
                    items={types.map(r => { return { id: r, title: r } })}
                    onSelectTab={tabId => setSelectedType(tabId)}
                >
                </Tabs>
                <ProfilesTable profiles={profiles} />
            </div>}
        </div>
    </div>
}

const ProfilesTable = ({ profiles }: { profiles: Array<ProfileList>}) => {
    if (!profiles || profiles.length == 0) {
        return <div />
    }

    const MyTable = withTableSorting(Table);
    const columns = [
        {
            id: 'TS', template: (item: any) => {
                return <div>
                    {new Date(item.TS).toLocaleString("ru-RU")}
                </div>
            }
        },
        {
            id: 'Lables', template: (item: any) => {
                return <HelpTooltip
                    offset={{
                        top: 2
                    }}
                    content={<ul>
                        {Object.keys(item.Labels).map(key => <li>{key}: {item.Labels[key]}</li>)}
                    </ul>}
                />
            }
        },
        {
            id: 'Links', template: (item: any) => {
                return <div>
                    {item.Links.map((link: any) => <Link target="blank" href={link.URL} className="link_btn"><Button>{link.Label}</Button></Link>)}
                </div>
            }
        },
    ];
    return <MyTable
        data={profiles}
        columns={columns}
    />
}
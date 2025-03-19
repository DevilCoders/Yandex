import React, {useEffect} from 'react'

interface IPageProps {
    title?: string
}

const Page: React.FC<IPageProps> = ({title, children}) => {
    useEffect(()=> {
        document.title = title ? "MDB UI: " + title : "MDB UI";
    }, [title]);
    return <>{children}</>
}

export default Page;

import React from "react";
import './Sidebar.css'

interface ISidebarProps {
    title: string
}
const Sidebar : React.FC<ISidebarProps> = ({title, children}) => {
    return (
        <div className="sidebar">
            <h2>{title}</h2>
            {children}
        </div>
    );
}

export default Sidebar;

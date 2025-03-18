<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:svg="http://www.w3.org/2000/svg" xmlns="http://www.w3.org/2000/svg">
<xsl:namespace-alias stylesheet-prefix="svg" result-prefix="#default"/>

<xsl:output
    method="xml"
    version="1.0"
    encoding="UTF-8"
    doctype-public="-//W3C//DTD SVG 1.1//EN"
    doctype-system="http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"
    indent="no"/>

<xsl:template match="/">
    <xsl:processing-instruction name="xml-stylesheet">type="text/css" href="style.css"</xsl:processing-instruction>
    <xsl:apply-templates/>
</xsl:template>

<xsl:template match="@*|node()">
    <xsl:copy>
        <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
</xsl:template>

<xsl:template match="/svg:svg">
    <xsl:copy>
        <xsl:apply-templates select="@*"/>
        <xsl:attribute name="version">1.1</xsl:attribute>
        <xsl:attribute name="width">100%</xsl:attribute>
        <xsl:attribute name="height">100%</xsl:attribute>
        <xsl:apply-templates select="node()"/>
    </xsl:copy>
</xsl:template>

<xsl:template match="svg:title"/>

<xsl:template match="svg:g[@class='graph']/svg:polygon[1]">
    <xsl:copy>
        <xsl:apply-templates select="@*"/>
        <xsl:attribute name="id">graph-background-node</xsl:attribute>
    </xsl:copy>
</xsl:template>

<xsl:template match="svg:g[@class='node']">
    <xsl:variable name="shape-nodes" select="svg:circle|svg:ellipse|svg:polygon"/>
    <xsl:copy>
        <xsl:copy-of select="@id"/>
        <xsl:attribute name="isNode">true</xsl:attribute>
        <clipPath id="{@id}-node-clip">
            <xsl:copy-of select="$shape-nodes"/>
        </clipPath>
        <g class="node" clip-path="url(#{@id}-node-clip)">
            <ellipse cx="{svg:text/@x}" cy="{svg:text/@y}" rx="180" ry="60" fill="none" stroke="none"/>
        </g>
        <xsl:copy-of select="$shape-nodes"/>
        <a class="node" target="_top">
            <xsl:copy-of select="svg:text"/>
        </a>
    </xsl:copy>
</xsl:template>

<xsl:template match="svg:g[@class='edge']/svg:path">
    <path visibility="hidden" pointer-events="stroke" stroke-width="10" d="{@d}"/>
    <xsl:copy>
        <xsl:apply-templates select="@*"/>
    </xsl:copy>
</xsl:template>

</xsl:stylesheet>

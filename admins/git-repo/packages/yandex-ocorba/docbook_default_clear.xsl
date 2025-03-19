<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output encoding="windows-1251"/>
	
	<!-- article -->
	<xsl:template match="article">
	<xsl:call-template name="anchor"/>
	<xsl:call-template name="anchor"/>
		<xsl:apply-templates/>
	</xsl:template>
	
	<!-- articleinfo -->
	<xsl:template match="articleinfo"><xsl:call-template name="anchor"/>
		<xsl:apply-templates/>
	</xsl:template>
	
	<!-- title -->
	<xsl:template match="title"><xsl:call-template name="anchor"/>
		<xsl:choose>
			<xsl:when test="@role = 'important' ">
				<b><xsl:apply-templates/></b>
			</xsl:when>
			<xsl:otherwise>
				<xsl:apply-templates/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	
	<xsl:template match="section/title"><xsl:call-template name="anchor"/>
	<h1><xsl:apply-templates/></h1>
	</xsl:template>
	
	<xsl:template match="section/subtitle"><xsl:call-template name="anchor"/>
	<h3><xsl:apply-templates/></h3>
	</xsl:template>
	
	<!-- section -->
	<xsl:template match="section"><xsl:call-template name="anchor"/>
		<div class="section"><xsl:apply-templates/></div>
	</xsl:template>
	
	<!-- para -->
	<xsl:template match="para"><xsl:call-template name="anchor"/>
		<div>
			<xsl:apply-templates/>
		</div>
	</xsl:template>
	
	<xsl:template match="section/para"><xsl:call-template name="anchor"/>
		<div>
			<xsl:apply-templates/>
		</div>
	</xsl:template>
	
	<xsl:template match="listitem/para"><xsl:call-template name="anchor"/>
		<xsl:apply-templates/>
	</xsl:template>
	
	<xsl:template match="listitem/para[@role = 'important' ]"><xsl:call-template name="anchor"/>
		<div>
			<b><xsl:apply-templates/></b>
		</div>
	</xsl:template>
	
	<!--  filename -->
	<xsl:template match="filename"><xsl:call-template name="anchor"/>
		<b>&#xAB;<xsl:apply-templates/>&#xBB;</b>
	</xsl:template>
	
	<!-- example -->
	<xsl:template match="example"><xsl:call-template name="anchor"/>
		<div>
			<xsl:apply-templates/>
		</div>
	</xsl:template>
	
	<!-- author -->
	<xsl:template match="author"><xsl:call-template name="anchor"/>
		<div><xsl:apply-templates/></div>
	</xsl:template>

	<!-- firstname -->
	<xsl:template match="firstname"><xsl:call-template name="anchor"/>
		<xsl:apply-templates/>
	</xsl:template>
	
     <!-- surname -->
	<xsl:template match="surname"><xsl:call-template name="anchor"/>
	     <xsl:text> </xsl:text>
		<xsl:apply-templates/>
	</xsl:template>
		
	<!-- quote -->
	<xsl:template match="quote"><xsl:call-template name="anchor"/>&#xAB;<xsl:apply-templates/>&#xBB;</xsl:template>

	<!-- systemitem -->
	<xsl:template match="systemitem[@class = 'osname' ] | systemitem"><xsl:call-template name="anchor"/>&#xAB;<xsl:apply-templates/>&#xBB;</xsl:template>

	<xsl:template match="systemitem[@class = 'resourse' ]"><xsl:call-template name="anchor"/><b><xsl:apply-templates/></b></xsl:template>
	
	<!-- xref -->
	<xsl:template match="xref"><xsl:call-template name="anchor"/><a href="?id={@linkend}"><xsl:value-of select="@endterm"/></a></xsl:template>
	
	<!-- ulink -->
	<xsl:template match="ulink"><xsl:call-template name="anchor"/><a href="{@url}"><xsl:copy-of select="@target"/><xsl:copy-of select="@onclick"/><xsl:apply-templates/></a></xsl:template>

	<xsl:template match="ulink[@type = 'hyper' ]"><xsl:call-template name="anchor"/><a href="?id={@url}"><xsl:apply-templates/></a></xsl:template>

	<!-- command -->
	<xsl:template match="command"><xsl:call-template name="anchor"/>
		<b>&#xAB;<xsl:apply-templates/>&#xBB;</b>
	</xsl:template>
	
	<!-- itemizedlist -->
	<xsl:template match="itemizedlist"><xsl:call-template name="anchor"/>
		<xsl:apply-templates select="title"/>
		<ul>
			<xsl:apply-templates select="listitem"/>
		</ul>
	</xsl:template>
	
	<!-- orderedlist -->
	<xsl:template match="orderedlist"><xsl:call-template name="anchor"/>
		<xsl:apply-templates select="title"/>
		<ol>
			<xsl:attribute name="type"><xsl:choose><xsl:when test="@numeration = 'arabic' ">1</xsl:when><xsl:when test="@numeration = 'loweralpha' ">a</xsl:when></xsl:choose></xsl:attribute>
			<xsl:apply-templates select="listitem"/>
		</ol>
	</xsl:template>
	
	<!-- listitem -->
	<xsl:template match="listitem"><xsl:call-template name="anchor"/>
		<li>
			<xsl:apply-templates/>
		</li>
	</xsl:template>
	
	<!-- table -->
	<xsl:template match="table"><xsl:call-template name="anchor"/>
		<xsl:apply-templates select="title"/>
		<table border="0" cellpadding="5" cellspacing="0">
			<xsl:apply-templates select="tgroup/thead | tgroup/tbody | tgroup/tfoot"/>
		</table>
		<br/>
	</xsl:template>
	<xsl:template match="thead | tbody | tfoot"><xsl:call-template name="anchor"/>
		<xsl:apply-templates select="row"/>
	</xsl:template>
	<xsl:template match="thead/row"><xsl:call-template name="anchor"/>
		<tr class="header">
			<xsl:apply-templates select="entry"/>
		</tr>
	</xsl:template>
	<xsl:template match="tbody/row"><xsl:call-template name="anchor"/>
		<tr valign="top" class="default">
			<xsl:apply-templates select="entry"/>
		</tr>
	</xsl:template>
	<xsl:template match="tfoot/row"><xsl:call-template name="anchor"/>
		<tr>
			<xsl:apply-templates select="entry"/>
		</tr>
	</xsl:template>
	<xsl:template match="entry"><xsl:call-template name="anchor"/>
		<td>
			<xsl:attribute name="colspan"><xsl:choose><xsl:when test="@namest and @nameend"><xsl:value-of select="count(../../preceding-sibling::colspec[@colname = current()/@nameend]) - count(../../../preceding-sibling::colspec[@colname = current()/@namest]) + 1"/></xsl:when></xsl:choose></xsl:attribute>
			<xsl:apply-templates/>
		</td>
	</xsl:template>
	
	<!-- note -->
	<xsl:template match="note"><xsl:call-template name="anchor"/>
		<div>
			<b>
				<xsl:apply-templates/>
			</b>
		</div>
	</xsl:template>
		
	<!-- phrase -->
	<xsl:template match="phrase"><xsl:call-template name="anchor"/>&#xAB;<xsl:apply-templates/>&#xBB;</xsl:template>
	
	<!-- email -->
	<xsl:template match="email"><xsl:call-template name="anchor"/>
		<a href="mailto:{text()}">
			<xsl:value-of select="text()|*[name() = $docbook_elements/element[@name = 'email']/element/@name]"/>
		</a>
	</xsl:template>
	
	<!-- footnote -->
	<xsl:template match="footnote"><xsl:call-template name="anchor"/>
		<div style="font-size: 90%; margin: 0.5 em 0 0.5em 0;">
			<xsl:apply-templates/>
		</div>
	</xsl:template>
	
	<!-- emphasis -->
	<xsl:template match="emphasis"><xsl:call-template name="anchor"/><b><xsl:apply-templates/></b></xsl:template>

		<!-- mediaobject -->
	<xsl:template match="mediaobject"><xsl:call-template name="anchor"/><xsl:apply-templates/></xsl:template>


		<!-- imageobject -->
	<xsl:template match="imageobject"><xsl:call-template name="anchor"/>	
	<xsl:apply-templates/></xsl:template>
	
	<!-- imagedata -->
	<xsl:template match="imagedata"><xsl:call-template name="anchor"/>	
	<img alt="" border="0" hspace="10">
	<xsl:attribute name="align"><xsl:value-of select="@align"/></xsl:attribute>
	<xsl:attribute name="valign"><xsl:value-of select="@valign"/></xsl:attribute>
	<xsl:attribute name="src"><xsl:value-of select="@fileref"/></xsl:attribute>
	</img>
</xsl:template>

	<!-- inlinegraphic -->
	<xsl:template match="inlinegraphic"><xsl:call-template name="anchor"/>	
	<img alt="" border="0" >
            <xsl:copy-of select="@width|@height"/>
	<xsl:attribute name="align"><xsl:value-of select="@align"/></xsl:attribute>
	<xsl:attribute name="valign"><xsl:value-of select="@valign"/></xsl:attribute>
	<xsl:attribute name="src"><xsl:value-of select="@fileref"/></xsl:attribute>
	</img>
</xsl:template>

    <!-- programlisting -->
    <xsl:template match="programlisting"><xsl:call-template name="anchor"/>
    <pre style="font-size: 120%"><xsl:apply-templates/></pre>
    </xsl:template>
    
    <!-- phone -->
    <xsl:template match="phone"><xsl:call-template name="anchor"/>	
    	<xsl:apply-templates/></xsl:template>

    <!-- qandaset -->
    <xsl:template match="qandaset"><xsl:call-template name="anchor"/>	
    	<xsl:apply-templates/></xsl:template>

    <!-- qandaentry -->
    <xsl:template match="qandaentry"><xsl:call-template name="anchor"/>	
	    <xsl:apply-templates/></xsl:template>

    <!-- qandadiv -->
    <xsl:template match="qandadiv"><xsl:call-template name="anchor"/>	
    	<xsl:apply-templates/></xsl:template>
  
    <!-- qandaentry -->  
    <xsl:template match="qandaentry"><xsl:call-template name="anchor"/>
        <xsl:apply-templates/>
</xsl:template>

    <!-- question -->  
    <xsl:template match="question"><xsl:call-template name="anchor"/>
      <xsl:apply-templates/>
</xsl:template>


    <!-- answer -->  
<xsl:template match="answer"><xsl:call-template name="anchor"/>
<xsl:apply-templates/>
</xsl:template>

<!-- variablelist -->
<xsl:template match="variablelist"><xsl:call-template name="anchor"/>
    <table xsl:use-attribute-sets="variablelistAttr">
    <xsl:apply-templates/>	
    </table>
</xsl:template>

<xsl:attribute-set name="variablelistAttr">
    <xsl:attribute name="border">0</xsl:attribute>
    <xsl:attribute name="cellpadding">0</xsl:attribute>
    <xsl:attribute name="cellspacing">0</xsl:attribute>
    <xsl:attribute name="width">100%</xsl:attribute>
    <xsl:attribute name="class">variableList</xsl:attribute>
</xsl:attribute-set>

<xsl:template match="variablelist/title">
	<tr><td xsl:use-attribute-sets="variablelistTitleAttr"><xsl:call-template name="anchor"/><xsl:apply-templates/></td></tr>
</xsl:template>
	
<xsl:attribute-set name="variablelistTitleAttr">
    <xsl:attribute name="colspan">2</xsl:attribute>
</xsl:attribute-set>
	
<xsl:template match="varlistentry"><xsl:call-template name="anchor"/>
	<tr valign="top"><td class="term"><xsl:apply-templates select="term"/></td><td class="listitem"><xsl:apply-templates select="listitem"/></td></tr>
</xsl:template>

<xsl:template match="varlistentry/listitem"><xsl:call-template name="anchor"/>
	<xsl:apply-templates/>
</xsl:template>

<xsl:template match="term"><xsl:call-template name="anchor"/><xsl:apply-templates/>
</xsl:template>

    <!-- guilabel -->  
    <xsl:template match="guilabel "><xsl:call-template name="anchor"/>
      <i><xsl:apply-templates/></i>
</xsl:template>

    <!-- guibutton -->  
    <xsl:template match="guibutton"><xsl:call-template name="anchor"/>
      <i><xsl:apply-templates/></i>
</xsl:template>

    <!-- parametr -->  
    <xsl:template match="parameter"><xsl:call-template name="anchor"/>&#xAB;<xsl:apply-templates/>&#xBB;</xsl:template>

    <!-- superscript -->  
    <xsl:template match="superscript"><xsl:call-template name="anchor"/><sup><xsl:apply-templates/></sup></xsl:template>

    <!-- glossary -->
    <xsl:template match="glossary">
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
    </xsl:template>
    
        <!-- glossdiv-->
    <xsl:template match="glossdiv|glossentry">
    <xsl:call-template name="anchor"/>
    <div><xsl:apply-templates/></div>
    <br clear="all"/>
    </xsl:template>
    
        <!-- glossentry -->
    <xsl:template match="glossterm|glossdef">
    <xsl:call-template name="anchor"/>
    <div class="{name()}"><xsl:apply-templates/></div>
        </xsl:template>

    <xsl:template match="glossdiv/title">
    <xsl:call-template name="anchor"/>
    <h3>&#151; <xsl:apply-templates/> &#151;</h3>
    </xsl:template>

	<xsl:template name="anchor">
	<xsl:if test="@id"><a><xsl:attribute name="name"><xsl:value-of select="@id"/></xsl:attribute></a></xsl:if>
	</xsl:template>
    

</xsl:stylesheet>

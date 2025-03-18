<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.1">
<xsl:output method="html" indent="yes" encoding="utf-8"/>

  
<!-- Если данный файл обрабатывается из Yandex.Software, -->
<!-- следующие параметры будут переписаны обработчиком   -->
<xsl:param name="script"></xsl:param>
<xsl:param name="images">i/</xsl:param>

<!-- Определим для удобства часто используемые переменные -->
<xsl:variable name="page" select="string(/yandexsearch/request/page)"/>
<xsl:variable name="reqid" select="string(/yandexsearch/response/reqid)"/>
<xsl:variable name="query" select="string(/yandexsearch/request/query)"/>

<!-- Формирование строки с группировкой -->
<xsl:variable name="grouping">mode=<xsl:value-of select="/yandexsearch/request/groupings/groupby/@mode"/>.attr=<xsl:value-of select="/yandexsearch/request/groupings/groupby/@attr"/>.groups-on-page=<xsl:value-of select="/yandexsearch/request/groupings/groupby/@groups-on-page"/>.docs-in-group=<xsl:value-of select="/yandexsearch/request/groupings/groupby/@docs-in-group"/>.curcateg=<xsl:value-of select="/yandexsearch/request/groupings/groupby/@curcateg"/></xsl:variable>

<xsl:variable name="grouping_esc">mode%3D<xsl:value-of select="/yandexsearch/request/groupings/groupby/@mode"/>.attr%3D<xsl:value-of select="/yandexsearch/request/groupings/groupby/@attr"/>.groups-on-page%3D<xsl:value-of select="/yandexsearch/request/groupings/groupby/@groups-on-page"/>.docs-in-group%3D<xsl:value-of select="/yandexsearch/request/groupings/groupby/@docs-in-group"/>.curcateg%3D<xsl:value-of select="/yandexsearch/request/groupings/groupby/@curcateg"/></xsl:variable>

<!-- Этот элемент не должен производить какой-либо вывод -->
<xsl:template match="request" />

<xsl:template name="printoption">
    <xsl:param name="optvalue" select="''"/>
    <xsl:param name="optcurrent" select="''"/>
    <xsl:param name="optname" select="''"/>
    <xsl:choose>
        <xsl:when test="$optvalue = $optcurrent">
            <option value="{$optvalue}" selected="selected"><xsl:value-of select="$optname"/></option>
        </xsl:when>
        <xsl:otherwise>
            <option value="{$optvalue}"><xsl:value-of select="$optname"/></option>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template name="printcheck">
    <xsl:param name="value" select="''"/>
    <xsl:param name="current" select="''"/>
    <xsl:param name="name" select="''"/>
    <xsl:choose>
        <xsl:when test="$value = $current">
           <input type="checkbox" name="{$name}" value="{$value}" checked="checked" />
        </xsl:when>
        <xsl:otherwise>
           <input type="checkbox" name="{$name}" value="{$value}" />
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template name="printradio">
    <xsl:param name="value" select="0"/>
    <xsl:param name="current" select="0"/>
    <xsl:param name="name" select="''"/>
    <xsl:choose>
        <xsl:when test="$value = $current">
           <input type="radio" name="{$name}" value="{$value}" checked="checked" />
        </xsl:when>
        <xsl:otherwise>
           <input type="radio" name="{$name}" value="{$value}" />
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>


<xsl:template match="querystring/param" mode="input">
<input type="hidden" name="{@name}" value="{text()}"/>
</xsl:template>

<xsl:template match="groupings" mode="input">
<xsl:text disable-output-escaping="yes">&lt;input type="hidden" name="groupby" value="</xsl:text><xsl:value-of select="$grouping" disable-output-escaping="yes"/><xsl:text>"/&gt;</xsl:text>
</xsl:template>

<!-- Здесь рисуем шапку, подвал и форму запроса -->
<xsl:template match="response">
<html>
<head>
	<title>Результаты поиска <xsl:value-of select="wordstat"/></title>
	<style>
		body, td, th, div {font-family: arial, tahoma, verdana; color: black}
		p {margin : 0;}
		.txtBigW {font-size: 100%; color: white}
		.txtMidW {font-size: 80%; color: white}
		.txtSmW {font-size: 75%; color: white}
		
                .txtFix10 {font-size: 10px; font-family: tahoma, arial, verdana;}
		.txtBig {font-size: 100%}
		.txtMid {font-size: 80%;}
		.txtSm {font-size: 75%;}
		
                a.w {color: white}
		a {color: #0764cb}
	</style>
</head>

<body bgcolor="#ffffff" style="margin: 5px;">

<p>
<form type="get" action="/q">
<table align="center" width="100%">
    <tr>
        <td width="50%" align="right"><input type="text" width="20" name="request"/></td>
        <td width="50%" align="left"><input type="submit" value="Найти!"/></td>
    </tr>
</table>
</form>
</p>

 <table cellpadding="0" cellspacing="0" border="0" width="760" align="center">
	<tr>
		<td class="txtMid">
			<img src="{$images}x.gif" width="760" height="1" border="0" vspace="10" style="background: #aaaaaa"/><br/>
      
			<b>Результаты поиска: <em><xsl:value-of select="$query"/>&#160;(<xsl:value-of select="(found[@priority='all'])"/>)</em></b><br/>
			<strong>Статистика слов:</strong>&#160;<xsl:value-of select="wordstat"/><br/><br/>
		   <xsl:apply-templates select="error"/>
          <xsl:apply-templates select="results"/>
      
  	<img src="i/x.gif" width="760" height="1" border="0" vspace="10" style="background: #aaaaaa"/><br/>
		</td>
	</tr>
</table>
<img src="{$images}x.gif" width="1" height="5" border="0"/><br/>
<!-- date sections -->
<!-- / date sections  -->

<!-- / footer -->

</body>
</html>
</xsl:template>

<xsl:template match="results">
    <xsl:apply-templates select="grouping"/>
</xsl:template>

<xsl:template name="pagelink">
    <xsl:param name="pagenum" select="0"/>
    <xsl:param name="curpage" select="0"/>
    <xsl:if test="$pagenum&gt;0">&#160;&#160;&#160;</xsl:if>
    <xsl:choose>
        <xsl:when test="$pagenum = $curpage">
            <strong><xsl:value-of select="$pagenum+1"/></strong>
        </xsl:when>
        <xsl:otherwise>
            <a href="{$script}?query={$query}&amp;groupby={$grouping_esc}&amp;page={$pagenum}">
            <xsl:value-of select="$pagenum+1"/></a>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>
 
<xsl:template name="pagelinks">
	<xsl:param name="pagemin" select="0"/>
    <xsl:param name="pagemax" select="0"/>
    <xsl:param name="pagenum" select="0"/>
    <xsl:param name="curpage" select="0"/>
    <xsl:call-template name="pagelink">
        <xsl:with-param name="pagenum" select="$pagenum"/>
        <xsl:with-param name="curpage" select="$curpage"/>
    </xsl:call-template>
    <xsl:if test="$pagenum&lt;$pagemax">
        <xsl:call-template name="pagelinks">
            <xsl:with-param name="pagenum" select="$pagenum + 1"/>
            <xsl:with-param name="pagemax" select="$pagemax"/>
            <xsl:with-param name="pagemin" select="$pagemin"/>
            <xsl:with-param name="curpage" select="$curpage"/>
        </xsl:call-template>
    </xsl:if>
</xsl:template>

<xsl:template match="error">

    <xsl:if test="string-length(.)&gt;0">
     <blockquote>  <p><b><xsl:value-of select="."/></b></p></blockquote>
    </xsl:if>

</xsl:template>
  
<xsl:template match="grouping">
    <xsl:variable name="curpage" select="number(page)"/>
    <xsl:variable name="groupcount" select="number(found[@priority='all'])"/>
    <xsl:variable name="first" select="number(page/@first)"/>
    <xsl:variable name="last" select="number(page/@last)"/>
    <xsl:variable name="pagemax" select="ceiling($groupcount div @groups-on-page)-1"/>
    <xsl:variable name="pagemin">
        <xsl:choose>
          <xsl:when test="number($curpage)-5&gt;0"><xsl:value-of select="number($curpage)-5"/></xsl:when>
          <xsl:otherwise><xsl:value-of select="0"/></xsl:otherwise>
       </xsl:choose>
    </xsl:variable>
    
    	<strong>
    <xsl:choose>
        <xsl:when test="@docs-in-group&gt;1"><xsl:text>Группы документов </xsl:text></xsl:when>
        <xsl:otherwise><xsl:text>Документы </xsl:text></xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="$first"/> - <xsl:value-of select="$last"/> из
    <xsl:value-of select="$groupcount"/>.
    </strong>
    <ol start="{$first}">
        <xsl:apply-templates select="group"/>
    </ol>
    <br clear="all"/>
    Сортировать по:
    <xsl:choose>
					<xsl:when test="../sortby = 'tm' "><a href="{$script}?query={$query}">релевантности</a>&#160;|&#160;<b>дате</b></xsl:when>
					<xsl:otherwise><b>релевантности</b>&#160;|&#160;<a href="{$script}?query={$query}&amp;groupby={$grouping_esc}&amp;sortby=tm">дате</a></xsl:otherwise>
				</xsl:choose><br/>
    
    <span>
    Страница результатов:&#160;
    <xsl:if test="$curpage&gt;0">
        <a HREF="{$script}?query={$query}&amp;groupby={$grouping_esc}&amp;page={number($curpage)-1}">&lt;&lt;&#160;Предыдущая</a>&#160;|&#160;        
    </xsl:if>
    <xsl:call-template name="pagelinks">
        <xsl:with-param name="curpage" select="$curpage"/>
        <xsl:with-param name="pagemin" select="$pagemin"/>
        <xsl:with-param name="pagenum" select="$pagemin"/>
        <xsl:with-param name="pagemax">
            <xsl:choose>
              <xsl:when test="$pagemax&gt;number($curpage)+5"><xsl:value-of select="number($curpage)+5"/></xsl:when>
              <xsl:otherwise><xsl:value-of select="$pagemax"/></xsl:otherwise>
           </xsl:choose>
        </xsl:with-param>
   </xsl:call-template>
   <xsl:if test="$last&lt;$groupcount">
      &#160;|&#160;<a HREF="{$script}?query={$query}&amp;groupby={$grouping_esc}&amp;page={number($curpage)+1}">Следующая&#160;&gt;&gt;</a>
   </xsl:if></span><br clear="all"/>
</xsl:template>

<xsl:template match="group">
    <li><xsl:apply-templates select="doc"/></li>
</xsl:template>

<xsl:template match="doc">
    <p>
    <a href="{url}" target="_blank"><xsl:apply-templates select="title"/></a>
    <br/>
    <xsl:apply-templates select="headline"/>
    <xsl:apply-templates select="passages"/>
    <br/><font size="-1" color="#8888888"><a href="{url}" target="_blank"><xsl:value-of select="url"/></a><xsl:text>, </xsl:text>
    <xsl:variable name="sz" select="number(size)"/>
    <xsl:choose>
      <xsl:when test="$sz&gt;1024">
        <xsl:value-of select="ceiling($sz div 1024)"/><xsl:text> Кб</xsl:text>
      </xsl:when>
      <xsl:otherwise>
          <xsl:value-of select="$sz"/><xsl:text> б</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:variable name="tm" select="string(modtime)"/>
    <xsl:variable name="year" select="substring($tm, 1, 4)"/>
    <xsl:variable name="month" select="substring($tm, 5, 2)"/>
    <xsl:variable name="day" select="substring($tm, 7, 2)"/>
    <xsl:variable name="hour" select="substring($tm, 10, 2)"/>
    <xsl:variable name="minute" select="substring($tm, 12, 2)"/>
    <xsl:variable name="second" select="substring($tm, 14, 2)"/>
    <xsl:text>, </xsl:text>
    <xsl:value-of select="$day"/><xsl:text>-</xsl:text>
    <xsl:value-of select="$month"/><xsl:text>-</xsl:text>
    <xsl:value-of select="$year"/><xsl:text>, </xsl:text>
 
    <xsl:value-of select="$hour"/><xsl:text>:</xsl:text>
    <xsl:value-of select="$minute"/><xsl:text>:</xsl:text>
    <xsl:value-of select="$second"/>
    </font>
    </p>
    <br/>
</xsl:template>


<xsl:template match="passages">
    <xsl:apply-templates select="passage"/>
</xsl:template>

<xsl:template match="passage">
    <p> *** <xsl:apply-templates/> </p>
</xsl:template>

<xsl:template match="headline">
    <xsl:apply-templates/>
</xsl:template>

<xsl:template match="hldoc">
    <xsl:value-of select="." disable-output-escaping="yes"/>
</xsl:template>

<xsl:template match="hlword">
<b><xsl:value-of select="." disable-output-escaping="yes"/></b>
<xsl:if test="following-sibling::*"><xsl:text> </xsl:text></xsl:if>
</xsl:template>


<xsl:template match="statistics">
<A NAME="YANDEX_BOTTOM"></A>
</xsl:template>

</xsl:stylesheet>


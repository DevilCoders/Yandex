<?xml version="1.0" encoding="windows-1251"?>
<!DOCTYPE xsl:stylesheet SYSTEM "symbols.ent">
<xsl:stylesheet version="1.0" 
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:date="http://exslt.org/dates-and-times"
exclude-result-prefixes="date"
>

<xsl:variable name="current_year" select="date:year()"/>

<!-- ������� �������� � ������
�������� ��������: project_start_year  - ��� ������� ������� 
 -->
<xsl:template name="copyright">
<xsl:param name="project_start_year"/>
<span>&copy;&nbsp;<xsl:choose>
	<xsl:when test="$project_start_year=$current_year"><xsl:value-of select="$project_start_year"/></xsl:when>
	<xsl:otherwise><xsl:value-of select="$project_start_year"/>&mdash;<xsl:value-of select="$current_year"/></xsl:otherwise>
</xsl:choose></span> &laquo;<a href="http://www.yandex.ru/">������</a>&raquo;
</xsl:template>

<!-- ������� ������ ��� ��� ���������
�������� ��������: project_start_year  - ��� ������� ������� 
 -->
<xsl:template name="copyright_year">
<xsl:param name="project_start_year"/>
<xsl:choose>
	<xsl:when test="$project_start_year=$current_year"><xsl:value-of select="$project_start_year"/></xsl:when>
	<xsl:otherwise><xsl:value-of select="$project_start_year"/>&mdash;<xsl:value-of select="$current_year"/></xsl:otherwise>
</xsl:choose>
</xsl:template>

</xsl:stylesheet>

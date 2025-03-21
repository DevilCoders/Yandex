static constexpr const char* defaultSchemes =
// Format: Scheme \t CountryCodeOffset \t CountryCodeLength \t AreaCodeOffset \t AreaCodeLength
    "_#-##-##	0	0	0	0\n"
    "_##-##-##	0	0	0	0\n"
    "_#-##-##-##	0	0	0	0\n"
    "_###_####	0	0	0	0\n"
    "_###_##_##	0	0	0	0\n"
    "_###-####	0	0	0	0\n"
    "_###-#-###	0	0	0	0\n"
    "_###-##-##	0	0	0	0\n"
    "_###-###-#	0	0	0	0\n"
    "(###-##-##	0	0	0	0\n"
    "(##)#-##-##	0	0	0	2\n"
    "(##)#_###_###	0	0	0	2\n"
    "(###)#####	0	0	0	3\n"
    "(###)#-##-##	0	0	0	3\n"
    "_###-######	0	0	0	3\n"
    "(###)######	0	0	0	3\n"
    "_###-###-####	0	0	0	3\n"
    "_###)###-##-##	0	0	0	3\n"
    "(+###)###_####	0	0	0	3\n"
    "(+###)###_##_##	0	0	0	3\n"
    "(+###)###-####	0	0	0	3\n"
    "(+###)###-##-##	0	0	0	3\n"
    "(###)#######	0	0	0	3\n"
    "(###)-#######	0	0	0	3\n"
    "(###)-###-##-##	0	0	0	3\n"
    "(###)##-#-##-##	0	0	0	3\n"
    "(###)##-###-##	0	0	0	3\n"
    "(###)###_####	0	0	0	3\n"
    "(###)###_##_##	0	0	0	3\n"
    "(###)###-####	0	0	0	3\n"
    "(###)###-#-###	0	0	0	3\n"
    "(###)###-##-##	0	0	0	3\n"
    "(###)###-###-#	0	0	0	3\n"
    "(###)####-###	0	0	0	3\n"
    "(###)###-#####	0	0	0	3\n"
    "(###)###-###-##	0	0	0	3\n"
    "(###)####-####	0	0	0	3\n"
    "(####)#-##-##	0	0	0	4\n"
    "(###-#)##-##-##	0	0	0	4\n"
    "(####)######	0	0	0	4\n"
    "(####)-######	0	0	0	4\n"
    "(####)-##_##_##	0	0	0	4\n"
    "(####)##_##_##	0	0	0	4\n"
    "(####)##-##-##	0	0	0	4\n"
    "(####)###-###	0	0	0	4\n"
    "(####)###-####	0	0	0	4\n"
    "(####)###-##-##	0	0	0	4\n"
    "(#####)#-##-#	0	0	0	5\n"
    "(##-###)#-##-##	0	0	0	5\n"
    "(###-##)#-##-##	0	0	0	5\n"
    "(###-##)##-###	0	0	0	5\n"
    "(#####)#####	0	0	0	5\n"
    "(#####)-#####	0	0	0	5\n"
    "(#####)#-##-##	0	0	0	5\n"
    "(#####)##-###	0	0	0	5\n"
    "(#####)###-###	0	0	0	5\n"
    "+#(##)###_###	0	1	0	2\n"
    "_#-##-#######	0	1	0	2\n"
    "_#(##)#######	0	1	0	2\n"
    "_#(###)#_##_##	0	1	0	3\n"
    "_#_###_#######	0	1	0	3\n"
    "_#_###_###-##-##	0	1	0	3\n"
    "_#_###-###-##-##	0	1	0	3\n"
    "_#-(###)-###-##-##	0	1	0	3\n"
    "_#-###_###-##-##	0	1	0	3\n"
    "_#-###-#######	0	1	0	3\n"
    "_#-###-##-#####	0	1	0	3\n"
    "_#-###-##-##-###	0	1	0	3\n"
    "_#-###-###-####	0	1	0	3\n"
    "_#-###-###-##-##	0	1	0	3\n"
    "_#-###-####-###	0	1	0	3\n"
    "_#(###)#######	0	1	0	3\n"
    "_#(###)##-##-###	0	1	0	3\n"
    "_#(###)###_####	0	1	0	3\n"
    "_#(###)###_##_##	0	1	0	3\n"
    "_#(###)###-####	0	1	0	3\n"
    "_#(###)###-##-##	0	1	0	3\n"
    "_####_###-##-##	0	1	0	3\n"
    "_####-###-##-##	0	1	0	3\n"
    "(+#_###)###-#-###	0	1	0	3\n"
    "(+#_###)###-##-##	0	1	0	3\n"
    "(#-###-###-##-##	0	1	0	3\n"
    "(#-###)###-####	0	1	0	3\n"
    "(#-###)###-##-##	0	1	0	3\n"
    "(#(###)###-##-##	0	1	0	3\n"
    "(####)#######	0	1	0	3\n"
    "+###########	0	1	0	3\n"
    "+#_###_#######	0	1	0	3\n"
    "+#_###_##_##_#_#_#	0	1	0	3\n"
    "+#_###_##-##-###	0	1	0	3\n"
    "+#_###_###_####	0	1	0	3\n"
    "+#_###_###_##_##	0	1	0	3\n"
    "+#_###_###-####	0	1	0	3\n"
    "+#_###_###-##-##	0	1	0	3\n"
    "+#_###-###-####	0	1	0	3\n"
    "+#_###-###-##-##	0	1	0	3\n"
    "+#-###-###-####	0	1	0	3\n"
    "+#-###-###-##-##	0	1	0	3\n"
    "+#(###)#######	0	1	0	3\n"
    "+#(###)###_####	0	1	0	3\n"
    "+#(###)###_##_##	0	1	0	3\n"
    "+#(###)###_##-##	0	1	0	3\n"
    "+#(###)###-####	0	1	0	3\n"
    "+#(###)###-#-###	0	1	0	3\n"
    "+#(###)###-##-##	0	1	0	3\n"
    "+#(###)####-###	0	1	0	3\n"
    "+####_###_##_##	0	1	0	3\n"
    "_#-###-###-##-##-##	0	1	0	3\n"
    "_#_####_##_##_##	0	1	0	4\n"
    "_#-(###-#)##-##-##	0	1	0	4\n"
    "_#-(####)-######	0	1	0	4\n"
    "_#-####-######	0	1	0	4\n"
    "_#-####-##-####	0	1	0	4\n"
    "_#(###-#)###-###	0	1	0	4\n"
    "_#(####)######	0	1	0	4\n"
    "_#(####)##_##_##	0	1	0	4\n"
    "_#(####)##-##-##	0	1	0	4\n"
    "_#(####)###-###	0	1	0	4\n"
    "(#_####)##-##-##	0	1	0	4\n"
    "(#-####)######	0	1	0	4\n"
    "(#-####)##-##-##	0	1	0	4\n"
    "+#_####_##-##-##	0	1	0	4\n"
    "+#(###-#)##-##-##	0	1	0	4\n"
    "+#(####)######	0	1	0	4\n"
    "+#(####)##-####	0	1	0	4\n"
    "+#(####)##-##-##	0	1	0	4\n"
    "+#(####)###_###	0	1	0	4\n"
    "+#(####)###-###	0	1	0	4\n"
    "+#(####)###-##-##	0	1	0	4\n"
    "+#(####)###########	0	1	0	4\n"
    "_#(###-##)#-##-##	0	1	0	5\n"
    "_#(#####)#####	0	1	0	5\n"
    "_#(#####)#-##-##	0	1	0	5\n"
    "_#(#####)##-###	0	1	0	5\n"
    "_#(#####)##-#-##	0	1	0	5\n"
    "_#(#####)###-##	0	1	0	5\n"
    "(+#_#####)##-#-##	0	1	0	5\n"
    "(#-#####)#-##-##	0	1	0	5\n"
    "+#(###_##)#-##-##	0	1	0	5\n"
    "+#(###-##)#-##-##	0	1	0	5\n"
    "+#(#####)#####	0	1	0	5\n"
    "+##_#-###_####	0	2	0	1\n"
    "+##-##-###_####	0	2	0	2\n"
    "+##########	0	2	0	3\n"
    "+##_###_###_###	0	2	0	3\n"
    "+##_###_####_##	0	2	0	3\n"
    "(+##)###_###_##_##	0	2	0	3\n"
    "+#_#_###_#######	0	2	0	3\n"
    "+#_#_###_###_##_##	0	2	0	3\n"
    "+##_###_#######	0	2	0	3\n"
    "+##_###_###_####	0	2	0	3\n"
    "+##_###_###_##_##	0	2	0	3\n"
    "+##_###_###-##-##	0	2	0	3\n"
    "+##-###-###-##-##	0	2	0	3\n"
    "+##(###)#######	0	2	0	3\n"
    "+##(###)###_##_##	0	2	0	3\n"
    "+##(###)###-####	0	2	0	3\n"
    "+##(###)###-##-##	0	2	0	3\n"
    "+##_###-########	0	2	0	3\n"
    "+##-####-##-##-##	0	2	0	4\n"
    "+##(####)######	0	2	0	4\n"
    "+##(####)##_##_##	0	2	0	4\n"
    "+##(####)##-##-##	0	2	0	4\n"
    "_##(#####)#####	0	2	0	5\n"
    "+##(#####)#-##-##	0	2	0	5\n"
    "+##(#)###_##_##_##	0	2	1	3\n"
    "+##(#)####_######	0	2	1	4\n"
    "+##(#)####_##-####	0	2	1	4\n"
    "+###_#######	0	3	0	0\n"
    "_###_#_#######	0	3	0	1\n"
    "(###-#)###-####	0	3	0	1\n"
    "(###)-#-#######	0	3	0	1\n"
    "+###_#_#######	0	3	0	1\n"
    "+###_#_###_####	0	3	0	1\n"
    "+###_#_####_###	0	3	0	1\n"
    "+###-#-####-###	0	3	0	1\n"
    "+###-#-####-####	0	3	0	1\n"
    "_###-##-#####	0	3	0	2\n"
    "(+###_##)##-##-##	0	3	0	2\n"
    "(+###_##)###-###	0	3	0	2\n"
    "+###_##_###_###	0	3	0	2\n"
    "(+###_##)###-##-##	0	3	0	2\n"
    "(+###-##)###_##_##	0	3	0	2\n"
    "(+###)##_###_##_##	0	3	0	2\n"
    "(###_##)#######	0	3	0	2\n"
    "(###-##)###-##-##	0	3	0	2\n"
    "(#####)###-##-##	0	3	0	2\n"
    "+############	0	3	0	2\n"
    "+###_##_#######	0	3	0	2\n"
    "+###_##_###_####	0	3	0	2\n"
    "+###_##_###_#_###	0	3	0	2\n"
    "+###_##_###_##_##	0	3	0	2\n"
    "+###_##_###-##-##	0	3	0	2\n"
    "+###-##-###-##-##	0	3	0	2\n"
    "+###(##)#######	0	3	0	2\n"
    "+###(##)###-##-##	0	3	0	2\n"
    "+#####_###_####	0	3	0	2\n"
    "+#####_###_##_##	0	3	0	2\n"
    "+#####-###-##-##	0	3	0	2\n"
    "+###_###_#####	0	3	0	3\n"
    "+###-###-###-##	0	3	0	3\n"
    "(+###_###)##_##_##	0	3	0	3\n"
    "(+###_###)##-##-##	0	3	0	3\n"
    "+###_###_######	0	3	0	3\n"
    "+###-###(##-##-##	0	3	0	3\n"
    "+###(###)##-##-##	0	3	0	3\n"
    "+###(####)#####	0	3	0	4\n"
    "_###(####)#####	0	3	0	4\n"
    "(###)####_#####	0	3	0	4\n"
    "+###(####)#-##-##	0	3	0	4\n"
    "+###-(#)#-#######	0	3	1	1\n"
    "+###-(#)##-#-######	0	3	1	2\n"
    "+##(###)###_##-###	2	3	0	3\n"
    "+##(###)###-#####	2	3	0	3\n"

    // New schemes
    "(+#)###-###-####	0	1	0	3\n"
    "(+##-##)#######	0	2	0	2\n"
    "(+###)########	0	3	0	0\n"
    "(+###-##)###-##-##	0	3	0	2\n"
    "(+###_##)###_##_##	0	3	0	2\n"
    "(+#####)###-##-##	0	3	0	2\n"
    "(+#####)#######	0	3	0	2\n"
    "(+##_###)###-##-##	0	2	0	3\n"
    "(+##_###)#######	0	2	0	3\n"
    "(+##_###)###_##_##	0	2	0	3\n"
    "(#-###)(#-#####)#-##-##	4	1	0	5\n"
    "(#-###)(#####)#-##-##	0	1	3	5\n"
    "(#-###)#-##-##	0	1	0	3\n"
    "(#-###)###_##_##	0	1	0	3\n"
    "(#-###-#)##-##-##	0	1	0	4\n"
    "(#-###-##)#-##-##	0	1	0	5\n"
    "_####-###_##_##	0	1	0	3\n"
    // "(+##)###_###_###	0	2	0	2\n" // ambig area code for different countries
    "(##)###-##-##	0	0	0	2\n"
    "(##)####-####	0	0	0	2\n"
    "(##)#######	0	0	0	2\n"
    "(##)########	0	0	0	2\n"
    "(##)####_####	0	0	0	2\n"
    "(##)###_##_##	0	0	0	2\n"
    "(###)-###-####	0	0	0	3\n"
    "(###)#-###-###	0	0	0	3\n"
    "(###)##-##-##	0	0	0	3\n"
    "(###)##-##-###	0	0	0	3\n"
    "(####)-##-##-##	0	0	0	4\n"
    "(####)##-####	0	0	0	4\n"
    "(####)###_###	0	0	0	4\n"
    "(####)###_####	0	0	0	4\n"
    "(#####)##-#-##	0	0	0	5\n"
    "(#####)##-##-##	0	0	0	5\n"
    "(#####)###-##	0	0	0	5\n"
    "(#####)######	0	0	0	5\n"
    "(#####)##_###	0	0	0	5\n"
    "(#####)#_##_##	0	0	0	5\n"
    "(######)#-##-##	0	1	0	5\n"
    "(#_###)###_##_##	0	1	0	3\n"
    "+#(###)#-###-###	0	1	0	3\n"
    "+#(###)##-##-###	0	1	0	3\n"
    "+#(###)##-###-##	0	1	0	3\n"
    "+#(###)###-###-#	0	1	0	3\n"
    "+#(####)##_##_##	0	1	0	4\n"
    "+#(#####)#-##-##	0	1	0	5\n"
    "+#(#####)##-###	0	1	0	5\n"
    "+#-(###)-###-##-##	0	1	0	3\n"
    "+#-###-##-##-###	0	1	0	3\n"
    "+#-###-#######	0	1	0	3\n"
    "+##(#)###_###_####	0	2	1	3\n"
    "+##(#)#_##_##_##_##	0	2	1	1\n"
    "+##(###)###_####	0	2	0	3\n"
    "+##-##-########	0	2	0	2\n"
    "+##-###-###-####	0	2	0	3\n"
    "+##-###-#######	0	2	0	3\n"
    "+##-###-########	0	2	0	3\n"
    "+###(#)###-##-##	0	3	0	1\n"
    "+###(##)###-####	0	3	0	2\n"
    "+###(##)###_####	0	3	0	2\n"
    "+###(##)###_##_##	0	3	0	2\n"
    "+###(###)###-##-##	0	3	0	3\n"
    "+###(###)######	0	3	0	3\n"
    "+###-#-#######	0	3	0	1\n"
    "+###-##-#######	0	3	0	2\n"
    "+####-###-##-##	0	1	0	3\n"
    "+#####-#######	0	3	0	2\n"
    "+#############	0	3	0	2\n"
    "+#####_###-##-##	0	3	0	2\n"
    "+#####_#######	0	3	0	2\n"
    "+###_########	0	3	0	2\n"
    "+###_###_###_###	0	3	0	3\n"
    "+###_###_##_##_##	0	3	0	3\n"
    "+###_##_######	0	3	0	2\n"
    "+###_##_#_###_###	0	3	0	2\n"
    "+##_#########	0	2	0	1\n"
    "+##_##########	0	2	0	3\n"
    "+##_####_######	0	2	0	4\n"
    "+##_####_###_###	0	2	0	4\n"
    "+##_##_#######	0	2	0	2\n"
    "+##_##_########	0	2	0	2\n"
    "+##_##_####_####	0	2	0	2\n"
    "+##_##_###_####	0	2	0	2\n"
    "+##_##_###_##_##	0	2	0	2\n"
    "+##_##_##_##_##	0	2	0	2\n"
    "+##_#_####_####	0	2	0	1\n"
    "+##_#_###_####	0	2	0	1\n"
    "+##_#_##_##_##_##	0	2	0	1\n"
    "+#_##########	0	1	0	3\n"
    "+#_####_######	0	1	0	4\n"
    "+#_####_##_##_##	0	1	0	4\n"
    "_#(##)###-##-##	0	1	0	2\n"
    "_#(###)-###-##-##	0	1	0	3\n"
    "_#(###)#-###-###	0	1	0	3\n"
    "_#(###)##-##-##	0	1	0	3\n"
    "_#(###)##-###-##	0	1	0	3\n"
    "_#(###)###-#-###	0	1	0	3\n"
    "_#(###)###-###-#	0	1	0	3\n"
    "_#(###-#)##-##-##	0	1	0	4\n"
    "_#(####)###_###	0	1	0	4\n"
    "_#(#####)#_##_##	0	1	0	5\n"
    "_#-(###)-###-####	0	1	0	3\n"
    "_#-(###)###-##-##	0	1	0	3\n"
    "_#-(####)-##-##-##	0	1	0	4\n"
    "_#-###-#-###-###	0	1	0	3\n"
    "_#-###-##-#-##-##	0	1	0	5\n"
    "_#-###-##-##-##	0	1	0	3\n"
    "_#-###-##-###-##	0	1	0	3\n"
    "_#-###-###-#-###	0	1	0	3\n"
    "_#-###-###-###	0	1	0	3\n"
    "_#-###-###-###-#	0	1	0	3\n"
    "_#-###-###_####	0	1	0	3\n"
    "_#-###-###_##_##	0	1	0	3\n"
    "_#-####-##-##-##	0	1	0	4\n"
    "_#-####-###-###	0	1	0	4\n"
    "_#-#####-#-##-##	0	1	0	5\n"
    "_#-##########	0	1	0	3\n"
    "_#-###_###-####	0	1	0	3\n"
    "_#-###_###_####	0	1	0	3\n"
    "_##-##-########	0	2	0	2\n"
    "_###(##)###-##-##	0	3	0	2\n"
    "_###-##-##-###	0	0	0	3\n"
    "(#####)#######	0	0	0	5\n"
    "(#_###)###-##-##	0	1	0	3\n"
    "+#-###-##-##-###	0	1	0	3\n"
    "+##(#)##_####_####	0	1	1	2\n"
    "+##(####)###-###	0	2	0	4\n"
    "+###(##)###_##_##	0	3	0	2\n"
    "+###(###)###-##-##	0	3	0	3\n"
    "+#####_###-##-##	0	3	0	2\n"
    "+##_#_###_####	0	2	0	1\n"
    "_#-####-###-###	0	1	0	4\n"
    "_###-###-##-##	0	0	0	3\n"
    "_###-###_##_##	0	0	0	3\n"
    "_####-###_##_##	0	1	0	3\n"
    "_####_###_####	0	1	0	3\n"
    "_####_###_##_##	0	1	0	3\n"
    "_###_###-##-##	0	0	0	3\n"
    "_###_###-####	0	0	0	3\n"
    "_###_#######	0	0	0	3\n"
    "_###_###_####	0	0	0	3\n"
    "_###_###_##_##	0	0	0	3\n"
    "_###_##_##_##	0	0	0	3\n"
    "_050_##_##_###	0	0	0	3\n"
    "_06#_##_##_###	0	0	0	3\n"
    "_09#_##_##_###	0	0	0	3\n"
    "_916_##_##_###	0	0	0	3\n"
    "_903_##_##_###	0	0	0	3\n"
    "_495_##_##_###	0	0	0	3\n"
    "_499_##_##_###	0	0	0	3\n"
    "_800_##_##_###	0	0	0	3\n"
    "_##_###_###_##_##	0	2	0	3\n"
    "_##_###_##_##	0	0	0	2\n"
    "_#_###-###_####	0	1	0	3\n"
    "_#_##########	0	1	0	3\n"
    "_#_####_##-##-##	0	1	0	4\n"
    "_#_###_####_###	0	1	0	3\n"
    "_#_###_###_####	0	1	0	3\n"
    "_#_###_###_##_##	0	1	0	3\n"
    "_#_###_###_#_###	0	1	0	3\n"
    "_#_###_##_##_###	0	1	0	3\n"
    "(##-#)####_####	0	2	0	1\n"
    "(####)###_##_##	0	1	0	3\n"
    "(####)###_#_###	0	1	0	3\n"
    "(####)##_####	0	0	0	4\n"
    "(#####)#-####	0	0	0	5\n"
    "(#####)###_##	0	0	0	5\n"
    "(#_###_###_####	0	1	0	3\n"
    "+##(#)####_####	0	2	0	1\n"
    "+##(#)##_###_##_##	0	2	1	2\n"
    "+###_####_#####	0	3	0	4\n"
    "+###_###_##-##-##	0	3	0	3\n"
    "+##_#####_#####	0	2	0	5\n"
    "+##_####_###_##_##	0	2	1	3\n"
    "+##_##_##_##_##	0	2	0	2\n"
    "+#_####_###_###	0	1	0	4\n"
    "_#(###)####-###	0	1	0	3\n"
    "_#(###_##)#-##-##	0	1	0	5\n"
    "_#-#####-#####	0	1	0	5\n"
    "_#_###-###-####	0	1	0	3\n"
    "+#(#####)#-####	0	1	0	5\n"
    "+##(##)##_##_####	0	2	0	2\n"

    // New weak schemes
    "_#_##_##	0	0	0	0	1\n"
    "_####-###-####	0	1	0	3	1\n"
    "_####-###-########	2	2	0	3	1\n"
    "_####-#######	0	1	0	3	1\n"
    //"_#####	0	0	0	0	1\n" // too noisy even with markers
    "_######	0	0	0	0	1\n"
    "_#-###-###	0	0	0	0	1\n"
    "_##(###)###-##-##	0	2	0	3	1\n"
    "_##(###)###-####	0	2	0	3	1\n"
    "_##(###)#######	0	2	0	3	1\n"
    "_##-#-##	0	0	0	0	1\n"
    "_##-##-###	0	0	0	0	1\n"
    "_##-###-##	0	0	0	0	1\n"
    "_##-###-#######	0	2	0	3	1\n"
    "_##-###-########	0	2	0	3	1\n"
    "_##-#######	0	0	0	2	1\n"
    "_###-##	0	0	0	0	1\n"
    "_###-###	0	0	0	0	1\n"
    "_##-#-##	0	0	0	0	1\n"
    "_##-##-###	0	0	0	0	1\n"
    "_###-##	0	0	0	0	1\n"
    "_###-###-##	0	0	0	0	1\n"
    "_###-###-###	0	0	0	0	1\n"
    "_###-#######	0	0	0	3	1\n"
    "_###-########	0	0	0	3	1\n"
    "_#######	0	0	0	0	1\n"
    "_########	0	0	0	0	1\n"
    "_#########	0	0	0	2	1\n"
    "_##########	0	0	0	3	1\n"
    "_###########	0	1	0	3	1\n"
    "_############	0	3	0	2	1\n"
    "_#####_######	0	0	0	0	1\n"
    "_#####_###_###	0	0	0	0	1\n"
    "_####_##-##-##	0	0	0	3	1\n"
    "_##-#-##	0	0	0	0	1\n"
    "_##-##-###	0	0	0	0	1\n"
    "_###-###-##	0	0	0	0	1\n"
    "_####_#######	0	1	0	3	1\n"
    "_####_##_##_##	0	1	0	3	1\n"
    "_###_######	0	0	0	3	1\n"
    "_###_####_####	0	0	0	3	1\n"
    "_###_###_###	0	0	0	3	1\n"
    "_##_####_####	0	0	0	2	1\n"
    "_##_##_##	0	0	0	0	1\n"
    "_##_##_##_##	0	0	0	2	1\n"
    "_##_##_##_##_##	0	0	0	2	1\n"
    "_#_####_######	0	0	0	2	1\n"
    "_#_###_###-####	0	1	0	3	1\n"
    "_#_###_###_###	0	1	0	3	1\n"
    "_##(###)#######	0	2	0	3	1\n"
    "_##(####)######	0	2	0	4	1\n"
    "_#####-#-##-##	0	0	0	5	1\n"
    "_####_###_#_###	0	0	1	3	1\n"
    "_###_########	0	0	0	3	1\n"
    "_###_#_###	0	0	0	0	1\n"
    "_##_########	0	0	0	2	1\n"
    "_#_###_###-###	0	1	0	3	1\n"
    "_##_##_###	0	0	0	0	1\n"
    "";

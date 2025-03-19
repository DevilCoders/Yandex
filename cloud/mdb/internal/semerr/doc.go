/*
	Package semerr implements errors with semantics.

	When error chain is searched for a semantic error, only the top-most one is reported. This allows overriding
	error semantic depending on the situation.

 	Semantic errors are not supposed to be sentinels and for that purpose they cannot wrap other errors when
	they are sentinels.
*/
package semerr

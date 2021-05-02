BEGINFILE {
	n = split(FILENAME, fncomps, /\//)
	fname = fncomps[n]
	gsub(/\.v\.h$/, ".h", fname)
}
/\/\/provide|\/\/require/ {
	incvar = "VINE_" gensub(/\./, "_", "g", toupper($2)) "_INCLUDED";
}
/\/\/require/ {
	print "#ifndef " incvar
	print "#error \"Must include " $2 " before including " fname "\""
	print "#endif"
	next
}
/\/\/provide/ {
	print "#ifdef " incvar
	print "#error \"May not include " $2 " more than once\""
	print "#endif"
	print "#define " incvar
	next
}
{print $0}

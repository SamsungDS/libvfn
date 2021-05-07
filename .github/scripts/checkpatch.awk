#!/usr/bin/awk -f

BEGIN { RS = ""; FS = "\n"; ERR = 0 }

{
	if (!match($1, /^(WARNING|ERROR): (.*)$/, msg)) {
		print $0 ORS
		next
	}

	ERR = 1

	if (msg[1] == "ERROR") {
		printf "::error"
	} else {
		printf "::warning"
	}

	if (NF > 1) {
		match($2, "^#(.*): FILE: (.*):(.*):$", where)
		printf " file=%s,line=%d::%s\n", where[2], where[3], msg[2]
	} else {
		printf "::%s\n", msg[2]
	}

	for (i = 2; i <= NF; i++) {
		print $i
	}

	printf ORS
}

END { exit ERR }

#!/usr/bin/sh
# mkstfs [database]

# Copyright 2020 James R.
# All rights reserved.
#
# Read the 'LICENSE' file.

function help {
	cat >&2 <<EOT
mkstfs [path]

Make a new STFS database at the given path.
If the path is omitted, it is created at '.stfs' in the current directory.

Copyright 2020 James. R
EOT
	exit 1
}

if [ $# -gt 1 ]
then
	help
elif [ $# -eq 1 ]
then
	case "$1" in
		-?|-h|--help)
			help
			;;
		-*)
			echo >&2 "invalid option: $1" ; exit 1
	esac
else
	set -- ".stfs"
fi

if [ -e "$1" ]
then
	echo >&2 "$1: file exists" ; exit 1
fi

sqlite3 "$1" <<EOT
CREATE TABLE \`inodes\`
(
	\`path\`   TEXT    NOT NULL UNIQUE

,	\`ctime\`  INTEGER NOT NULL
,	\`mtime\`  INTEGER NOT NULL

,	\`master\` INTEGER
,	\`page\`   INTEGER

,	UNIQUE (\`master\`,\`page\`)
);

CREATE TABLE \`mappings\`
(
	\`inode\` INTEGER NOT NULL
,	\`tag\`   INTEGER NOT NULL
);

CREATE TABLE \`tags\`
(
	\`name\` TEXT    NOT NULL UNIQUE
,	\`time\` INTEGER NOT NULL
);

CREATE TABLE \`inheritance\`
(
	\`parent\` INTEGER NOT NULL
,	\`child\`  INTEGER NOT NULL

,	UNIQUE (\`parent\`,\`child\`)
);
EOT

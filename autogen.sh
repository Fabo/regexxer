#! /bin/sh
#
# Copyright (C) 2002 Daniel Elstner <daniel.elstner@gmx.net>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

dir=`echo "$0" | sed 's,[^/]*$,,'`
test "x${dir}" = "x" && dir='.'

if test "x`cd "${dir}" 2>/dev/null && pwd`" != "x`pwd`"
then
    echo "This script must be executed directly from the source directory."
    exit 1
fi

rm -f config.cache acconfig.h

echo "- aclocal."		&& \
aclocal -I ./macros		&& \
echo "- autoconf."		&& \
autoconf			&& \
echo "- autoheader."		&& \
autoheader			&& \
echo "- automake."		&& \
automake --add-missing --gnu	&& \
echo				&& \
./configure "$@"		&& exit 0

exit 1


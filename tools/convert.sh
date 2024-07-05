#!/bin/bash
# This shell script used to convert a HTML webpage to C source code string.

if [ $# != 1 ] ; then
	echo "Usage: $0 xxx.html"
	exit;
fi

HTML=$1
FNAME=`echo $HTML | cut -d. -f1`.c

cp $HTML $FNAME

# Remove all the blank line
sed -i '/^\s*$/d'    $FNAME

# Remove all the blank charactor in the end of the line
sed -i 's/\s*$//g'   $FNAME

# Replace all " to \"
sed -i 's/\"/\\\"/g' $FNAME

# Add \r\n" to the end of per line
sed -i 's/$/&\\r\\n"/g'  $FNAME

# Add " to the start of per line
sed -i 's/^/&\"/g'  $FNAME

# Add variable define in the first line
sed -i '1iconst char \*g_index_html='       $FNAME

# Add "; after the last line
sed -i '$a;'       $FNAME

# Add description in the first line
sed -i '1i\/\* This file is convert from HTML by tools/convert.sh \*\/'       $FNAME


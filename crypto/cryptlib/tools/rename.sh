#!/bin/sh
# Moves all OpenSSL symbols into their own namespace to avoid conflicts if
# linking with both cryptlib and OpenSSL.  The Mac version of sed is somewhat
# special and needs its own custom invocation.

if [ "$(uname)" = "Darwin" ] ; then
	LANG=C find ./ -name '*.h' -type f -exec sed -i '' -f tools/patterns.sed '{}' \;
	LANG=C find ./ -name '*.c' -type f -exec sed -i '' -f tools/patterns.sed '{}' \;
else
	LANG=C find ./ -name '*.h' -type f -exec sed -i -f tools/patterns.sed '{}' \;
	LANG=C find ./ -name '*.c' -type f -exec sed -i -f tools/patterns.sed '{}' \;
fi

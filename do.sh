#!/bin/sh

echo make
make
echo ./mytftpclient
./mytftpclient
#-W -d directory/file.txt -t 5005 -s 512 -m -c ascii -a 127.0.0.1,69
#!/bin/sh

make
echo -W -d file.txt -t 5005 -s 512 -m -c ascii -a 127.0.0.1,69 | ./mytftpclient

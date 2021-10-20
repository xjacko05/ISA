#!/bin/sh

make
echo -R -d test.txt -t 5005 -s 512 -m -c netascii -a 127.0.0.1,69 | ./mytftpclient

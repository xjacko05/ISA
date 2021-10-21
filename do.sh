#!/bin/sh

make
#echo -R -d doc.pdf -t 500 -s 256 -m -c octet -a 127.0.0.1,69 | ./mytftpclient
echo -R -d doc.pdf -s 2048 -t 550 -m -c octet -a 127.0.0.1,69 | ./mytftpclient

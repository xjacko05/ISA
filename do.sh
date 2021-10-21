#!/bin/sh

make
#echo -R -d doc.pdf -t 500 -s 256 -m -c octet -a 127.0.0.1,69 | ./mytftpclient
echo -W -d doc.pdf -s 512 -t 550 -m -c octet -a 127.0.0.1,69 | ./mytftpclient

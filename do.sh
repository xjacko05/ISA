#!/bin/sh

#make
#echo -R -d doc.pdf -t 500 -s 256 -m -c octet -a 127.0.0.1,69 | ./mytftpclient
#echo -W -d OGcat.jpeg -s 512 -m -c octet -a 0:0:0:0:0:FFFF:7F00:0001,69 | ./mytftpclient
echo -W -d OGdoc.pdf -t 10 -s 64 -m -c octet -a 0:0:0:0:0:FFFF:7F00:0001,69 | ./mytftpclient

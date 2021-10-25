#!/bin/sh

make
#echo -R -d doc.pdf -t 500 -s 256 -m -c octet -a 127.0.0.1,69 | ./mytftpclient
echo -R -d cat.jpeg -s 512 -m -c octet -a 127.0.0.1,69 | ./mytftpclient

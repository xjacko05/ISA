#!/bin/sh

#make
#echo -R -d doc.pdf -t 500 -s 256 -c octet -a 127.0.0.1,69 | ./mytftpclient
#echo -W -d OGcat.jpeg -s 512 -m -c octet -a 0:0:0:0:0:FFFF:7F00:0001,69 | ./mytftpclient
#echo -R -d simoncatto.jpg -t 10 -s 512 -m -c netascii -a 147.229.220.143,69 | ./mytftpclient
echo -R -d doc.pdf -s 512 -c netascii -a 127.0.0.1,69 | ./mytftpclient
-W -d OGcat.jpeg -s 512 -a 0:0:0:0:0:FFFF:7F00:0001,69

-R -d doc.pdf -c netascii
-W -d OGdoc.pdf
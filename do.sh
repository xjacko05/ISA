#!/bin/sh

#make
#echo -R -d doc.pdf -t 500 -s 256 -m -c octet -a 127.0.0.1,69 | ./mytftpclient
#echo -W -d OGcat.jpeg -s 512 -m -c octet -a 0:0:0:0:0:FFFF:7F00:0001,69 | ./mytftpclient
#echo -R -d simoncatto.jpg -t 10 -s 512 -m -c netascii -a 147.229.220.143,69 | ./mytftpclient
echo -W -d /home/student/Desktop/ISA/OGsimoncatto.jpg -s 55 -m -c netascii -a 127.0.0.1,69 | ./mytftpclient

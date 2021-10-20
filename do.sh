#!/bin/sh

make
echo -R -d pacman.png -t 5005 -s 512 -m -c octet -a 127.0.0.1,69 | ./mytftpclient

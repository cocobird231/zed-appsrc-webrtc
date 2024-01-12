#!/bin/bash
printf "%s" "Internet connecting..."
while ! ping -w 1 -c 1 -n 168.95.1.1 &> /dev/null
do
    printf "%c" "."
done
printf "\n%s\n" "Internet connected."
# Process here
./build/sendrecvzed --cam-id=0 --stream-type=0 --our-id=1 --peer-id=0 --turn
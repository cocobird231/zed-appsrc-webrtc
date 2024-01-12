#!/bin/bash
printf "%s" "Internet connecting..."
while ! ping -w 1 -c 1 -n 168.95.1.1 &> /dev/null
do
    printf "%c" "."
done
printf "\n%s\n" "Internet connected."
# Process here
./build/sendrecvzed --our-id=0 --remote-offerer=TRUE --turn
#!/bin/bash

cd /proj/trinity-PG0/Trinity/build/
make

for i in {1..9}
do
    ssh -o StrictHostKeyChecking=no -i /proj/trinity-PG0/Trinity/scripts/key -l Ziming 10.10.1.$(($i + 2)) "/proj/trinity-PG0/Trinity/build/librpc/TrinityGithubLookup $i" &
done

/proj/trinity-PG0/Trinity/build/librpc/TrinityGithubLookup 0

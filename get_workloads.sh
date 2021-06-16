#!/bin/bash


mkdir workloads/

echo -e "Downloading DAMOV workloads -- Part 1"
git clone https://damovsafari@bitbucket.org/damovsafari/damovworkloadspart1.git
cd damovworkloadspart1/
lrzuntar workloads_part1.tar.lrz
rm workloads_part1.tar.lrz
mv workloads_part1/* ../workloads
cd ..
rm -rf damovworkloadspart1


echo -e "\nDownloading DAMOV workloads -- Part 2"
git clone https://damovsafari@bitbucket.org/damovsafari/damovworkloadspart2.git
cd damovworkloadspart2/
lrzuntar workloads_part2.tar.lrz
rm workloads_part2.tar.lrz
mv workloads_part2/* ../workloads
cd ..
rm -rf damovworkloadspart2


echo -e "\nDownloading DAMOV workloads -- Part 3"
git clone https://damovsafari@bitbucket.org/damovsafari/damovworkloadspart3.git
cd damovworkloadspart3/
lrzuntar workloads_part3.tar.lrz
rm workloads_part3.tar.lrz
mv workloads_part3/* ../workloads
cd ..
rm -rf damovworkloadspart3

clear
ls workloads/




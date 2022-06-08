
echo -e "Downloading megatools"
wget https://megatools.megous.com/builds/experimental/megatools-1.11.0-git-20220401-linux-x86_64.tar.gz

tar -xvf megatools-1.11.0-git-20220401-linux-x86_64.tar.gz 

rm megatools-1.11.0-git-20220401-linux-x86_64.tar.gz

echo -e "\nDownloading DAMOV workloads"
./megatools-1.11.0-git-20220401-linux-x86_64/megatools dl https://mega.nz/file/Mz51xJyY#J_ai3_Pl5kVvFETurKmBuMIrOagUK4sadyahOzUYQVE

rm -rf megatools-1.11.0-git-20220401-linux-x86_64/

tar xvf damov_workloads.tar.gz 

rm damov_workloads.tar.gz

echo -e "\nDone"

clear
ls workloads/

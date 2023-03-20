echo -e "\nDecompressing MegaTools API"

tar -xvf megatools-1.11.1.20230212-linux-x86_64.tar.gz

echo -e "\nDownloading DAMOV workloads"
./megatools-1.11.1.20230212-linux-x86_64/megatools dl https://mega.nz/file/Mz51xJyY#J_ai3_Pl5kVvFETurKmBuMIrOagUK4sadyahOzUYQVE

echo -e "\nDecompressing DAMOV workloads"

tar xvf damov_workloads.tar.gz 

rm damov_workloads.tar.gz

echo -e "\nDone"

clear
ls workloads/

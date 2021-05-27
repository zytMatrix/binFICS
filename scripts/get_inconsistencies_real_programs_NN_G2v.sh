#!/bin/bash

benchmark=$1
preprocess=$2
split=$3

echo "[*]benchmark: "  ${benchmark}
echo "[*]preprocess: " ${preprocess}

if [ -z "${benchmark}" ]; then
    echo "benchmark is unset or set to the empty string"
    exit 1;
fi

if [ -z "${preprocess}" ]; then
    echo "No preprocessing"
    preprocess="np"
fi

if [ "${preprocess}" = "p" ]; then
	datasets=$(cat ./settings.py | grep "DATASETS_DIR" | cut -d '=' -f 2 | cut -d '#' -f 1 | tr -d \'\" | tr -d '[:space:]')
	bcs=$(cat ./settings.py | grep "BCS_DIR" | cut -d '=' -f 2 | cut -d '#' -f 1 | tr -d \'\" | tr -d '[:space:]')
	data=$(cat ./settings.py | grep "DATA_DIR" | cut -d '=' -f 2 | cut -d '#' -f 1 | tr -d \'\" | tr -d '[:space:]')
    echo "[*]datasets:   $datasets"
    echo "[*]bcs:  $bcs"
    echo "[*]data: $data"
    
	echo "Removing dataset folder of $data/$datasets/$benchmark"
	rm -rf "$data/$datasets/$benchmark"
	echo "Removing IR folder of $data/$bcs/$benchmark"
	rm -rf "$data/$bcs/$benchmark"


    echo "[*] generate .bc, .ll and save function information"
	python __init__.py -p=$benchmark -a=BC ## action BC = 'Retrieve bitcode'

    echo "[*] retrieve pdg"
	python __init__.py -p=$benchmark -a=PDG ## action PDG = 'Retrieve Program Dependence Graph'

    echo "[*] Extract Abstract Forward Slices"
	python __init__.py -p=$benchmark -a=AS -s=True ## action AS = 'Extract Abstract Forward Slices'
    
	if [ "${split}" = "ns" ]; then
        echo "[*] Feature Extraction"
		python __init__.py -p=$benchmark -a=FE -ft=afs_NN ## FE: Feature Extraction
		# python __init__.py -p=$benchmark -a=FE -ft=afs.bb1_NN
	else
		python __init__.py -p=$benchmark -a=FE -ft=afs_NN -s=True
        # python __init__.py -p=$benchmark -a=FE -ft=afs.bb1_NN -s=True
	fi
fi
exit 1;

## MC: Model Construction
## cf: clustering features
## ca: clustering algorithms and their thresholds
## sc: Type of second clustering, online vs offline
python __init__.py -p=$benchmark -a=MC -cf=afs_NN,afs_G2v -ca=cc_0.95,cc_0.98 -sc=online
exit 1;

python __init__.py -p=$benchmark -a=MC -cf=afs.bb1_NN,afs.bb1_G2v -ca=cc_0.95,cc_0.98 -sc=online

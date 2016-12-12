run_klee(){
    covnum = 0
    echo "" > $5
    int=1
    while(( $int<=$1 ))
    do
        klee -dataflow-testing-with=$2 -max-depth=50 \
        -def-use-pair-id=$int $4 $6 $7 $3 >> $5 2>>/dev/null
        if [ $? == 1 ] 
        then    
            let "covnum++"
        fi
        let "int++"
    done
    echo -e "\nCoverage: " >> $5
    echo "Covered: $covnum" >> $5
    echo "Total: $1" >> $5
    echo -n "Rate: " >> $5    
    echo "scale=4;$covnum / $1" | bc >> $5
}

(time run_klee $1 $2 $3 $4 $5 $6 $7) 2>> $5

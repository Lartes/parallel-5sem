#!/bin/sh

count() {
x=$(echo "scale=1; $n * 0.1 - 0.1" | bc)
rm config
echo dx = 0.1 $x >> config
echo dy = 0.1 $x >> config
echo dt = 0.01 >> config
echo tWrite = 0.1 >> config
echo tEnd = 1.0 >> config
echo test_ >> config
echo $n x $n
./simple
for i in 1 2 3 4
do
echo $i
num=$(($i * $i))
mpiexec -n $num ./main $i $i
done
echo done
}


series() {
echo seq,1,2,3,4, >> time.csv
for n in 10 100 500 1000 3000 5000
do
count
echo >> time.csv
done
}


rm time*
for m in 1 2 3 4 5 6 7 8 9 10
do
echo start №$m
series
mv time.csv "time$m.csv"
done 

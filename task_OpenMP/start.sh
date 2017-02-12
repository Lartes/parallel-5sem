#!/bin/sh

count() {
./gen $n $n
echo $n x $n gen
./simple
for i in 1 2 3 4 6 8 16 24 32
do
./main $i
done
echo done
}


series() {
echo seq,1,2,3,4,6,8,16,24,32, >> data.csv
for n in 3 10 100 500 1000 3000 5000 10000
do
count
echo >> data.csv
done
}


rm data.csv
rm time.csv
echo 3,10,100,500,1000,3000,5000,10000, >> time.csv
for m in 1 2 3 4 5 6 7 8 9 10
do
echo start №$m
series
mv data.csv "data$m.csv"
echo >> time.csv
done 

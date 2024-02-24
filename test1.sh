ls -al > listing1 & ls -al | sort > listing2 &
sleep 2

cat listing1 listing2 | sort > sorted
echo There are 
grep total sorted | wc -l
echo totals!
sleep 2

ls -al | shuf > listing1
ls -al | shuf > listing2
cat listing1 listing2 | sort | uniq > uniq_sorted
ls -al | shuf > listing1
ls -al | shuf > listing2
cat listing1 listing2 | sort | uniq > uniq_sorted
echo There is 
grep total uniq_sorted | wc -l
echo total!

sleep 2
more uniq_sorted
rm listing1 listing2 sorted uniq_sorted




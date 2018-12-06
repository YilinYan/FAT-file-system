folder=mountpoint
nr=10

echo -----------create $nr folders----------------

for k in `seq 0 $nr`; do
	mkdir $folder/folder$k
done

ls -la $folder

echo -----create a text file in each folder-------

for k in `seq 0 $nr`; do
	echo this is $k > $folder/folder$k/$k.txt
	ls -la $folder/folder$k
done

echo ---------show each text file ----------------

for k in `seq 0 $nr`; do
	cat $folder/folder$k/$k.txt
done

echo ---------delete each text file --------------

for k in `seq 0 $nr`; do
	rm $folder/folder$k/$k.txt
	ls -la $folder/folder$k
done

echo ------------delete each folder --------------

for k in `seq 0 $nr`; do
	rmdir $folder/folder$k
done

ls -la $folder

folder=mountpoint

for k in `seq 0 50`; do
	echo $k > $folder/$k
done

ls -la $folder

for k in `seq 0 50`; do
	cat $folder/$k
done


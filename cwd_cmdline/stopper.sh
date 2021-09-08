mkdir $1
cp /proc/$1/cmdline $1
cp -r /proc/$1/cwd $1 
kill -9 $1
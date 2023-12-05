then echo start

ls my*

pwd

echo hello

mkdir testdir

cd testdir

pwd

cd ..

ls

which ls

which exit

ls

ls -l > out.txt

cat < out.txt

echo HI

cat < out.txt > out2.txt

then ls -l out.txt

fail 

then ls

fail

else echo test

else ls

ls | grep out.txt
exit
out=aac2pcm
obj=aac2pcm.c

$(out):$(obj)
	gcc -o $(out) $(obj) -lfaad -L./libfaad
clean:
	rm -rf $(out) *.o

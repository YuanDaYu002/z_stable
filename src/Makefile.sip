
ipc:
	make -f ipcMakefile
webs:
	make -f wsMakefile
nvr:
	make -f nvrMakefile
svn:
	./create_version.sh
	cp p2p_interface.h ../include
	cp p2p_alarm.h ../include
	make -f ipcMakefile
svn_nvr:
	./create_version.sh
	make -f nvrMakefile
clean:
	rm -rf ./obj/*.o ./obj/sip/*.o ./obj/ws/*.o
	rm -rf libzsip.a libmeshare.a

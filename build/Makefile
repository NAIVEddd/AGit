vpath = ../src/
srcfiles := ../src/git-packfile.cpp ../src/git-reference.cpp ../src/git-client.cpp ../src/git-pack-line.cpp ../src/git-url.cpp ../main.cpp

agit :
	g++ $(srcfiles) -g -o agit -L/usr/local/lib -lz 

clean:
	rm agit
vpath = ../src/
srcfiles := ../src/git-repo.cpp ../src/git-patch.cpp ../src/git-path-helper.cpp ../util/sha1.cpp ../src/git-packfile.cpp ../src/git-reference.cpp ../src/git-client.cpp ../src/git-pack-line.cpp ../src/git-url.cpp ../main.cpp

agit :
	g++ -std=c++17 $(srcfiles) -g -o agit -L/usr/local/lib -lz 

clean:
	rm agit
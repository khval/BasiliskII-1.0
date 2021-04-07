
all: basilisk.ct updated

basilisk.ct: basilisk.cd
	CATCOMP basilisk.cd CTFILE basilisk.ct

updated: basilisk.ct
	protect update +s
	update
	date > updated

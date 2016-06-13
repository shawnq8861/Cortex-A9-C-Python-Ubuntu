all:
	cd source; make
	cd ..

clean:

install:
	mkdir -p $(DESTDIR)/opt/kymeta/lib
	cd source; cp *.so $(DESTDIR)/opt/kymeta/lib
	cd ..
	mkdir -p $(DESTDIR)/opt/kymeta/rowandcolumndriver
	cd scripts; cp *.py $(DESTDIR)/opt/kymeta/rowandcolumndriver
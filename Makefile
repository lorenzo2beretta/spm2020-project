CXX		= g++ -std=c++17 
CXXFLAGS  	= -g
LDFLAGS 	= -pthread
OPTFLAGS	= -O3 -finline-functions -DNDEBUG

TARGETS		= 	pthread-async	\
			pthread-sync 	\
			openmp 		\
			ff-parfor 	\
			sequential

.PHONY: all clean cleanall
.SUFFIXES: .cpp


%: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

all		: $(TARGETS)

clean		: 
	rm -f $(TARGETS)

cleanall	: clean
	\rm -f *.o *~




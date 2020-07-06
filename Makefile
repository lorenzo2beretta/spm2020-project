CXX		= g++ -std=c++17 
CXXFLAGS  	= -g -DNDEBUG -DTRACE_FASTFLOW
LDFLAGS 	= -pthread
OPTFLAGS	= -O3 -finline-functions 

TARGETS		=	ff-farm		\
			pthread-async	\
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




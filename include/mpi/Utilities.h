#ifndef INCLUDE_MPI_UTILITIES_H
#define INCLUDE_MPI_UTILITIES_H

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include "Basic.h"
#include "../JsonX.h"

//--------------------------------------------------schö tö tiä tü mö tiä par la barbischätöööötötötötö-------------------------------------------------------

namespace mpi {		
		
    inline jsx::value read(std::string name) {
		int size; std::string buffer;
		
                if(rank() == master) {
			std::ifstream file(name.c_str()); 
			if(!file) throw std::runtime_error("mpi: file " + name + " not found !");
			
			file.seekg(0, std::ios::end);
			size = file.tellg();
			file.seekg(0, std::ios::beg);
			
            buffer.resize(size + 1);
            file.read(&buffer.front(), size);
            buffer[size] = '\0'; ++size;
            
            file.close();
        }
        
        bcast(size, master);
        if(rank() != master) buffer.resize(size);
        bcast(buffer, master);
		
        jsx::value value;
        if(*jsx::parse(&buffer.front(), value) != '\0') throw std::runtime_error("json: file contains more ...");
        return value;
	}
    
    inline jsx::value read(std::string name, jsx::value dvalue) {
        int isFile = 1;
        
        if(rank() == master)
            if(!std::ifstream(name.c_str())) isFile = 0;
        
        bcast(isFile, master);
        return isFile ? read(name) : dvalue;
    }
	
    template<typename T>
    inline void write(T const& t, std::string name, std::size_t precision = 10) {
		if(rank() == master) {
			std::ofstream file(name.c_str());
            file << std::setprecision(precision);
			jsx::write(t, file);
			file.close();
        } else {
            std::ostringstream dummy;
            jsx::write(t, dummy);
        }
        
        barrier();  //Why the hell this ?!?!??!
	}

    
    enum class cout_mode { every, one, none };
    
    struct Cout {
        Cout() : mode_(cout_mode::one) {};
        void operator=(cout_mode mode) {mode_ = mode;};
        std::ostream& operator<<(std::ostream& (*pf)(std::ostream&)) {
            if(mode_ == cout_mode::every) return pf(std::cout);
            if(mode_ == cout_mode::one) return rank() == master ? pf(std::cout) : null_;
            return null_;
        };
    private:
        static std::ostream null_;
        cout_mode mode_;
        
        template<typename T>
        friend std::ostream& operator<<(Cout const&, T);
    };
    
    std::ostream Cout::null_(0);
    
    template<typename T>
    std::ostream& operator<<(Cout const& c, T t) {
        if(c.mode_ == cout_mode::every) return std::cout << t;
        if(c.mode_ == cout_mode::one) return rank() == master ? std::cout << t : Cout::null_;
        return Cout::null_;
    };
    
    //extern Cout cout;
    Cout cout;


    jsx::value mpi_structure(){
        
        jsx::value jMPIStructure;
        
        std::vector<int> rankOnNode(number_of_workers());
        std::vector<int> rankOfNode(number_of_workers());
        int nNodes=0;
        
        std::vector<char> nodeNames = processor_name();
        gather(nodeNames, master);
        
        if(rank() == master) {
            
            std::map<std::string,int> rank_on_node;
            std::map<std::string,int> rank_of_node;
            
            for(int rank = 0; rank < number_of_workers(); ++rank) {
                std::string const nodeName(&nodeNames[rank*processor_name_size()], processor_name_size());
            
                if(!rank_on_node.count(nodeName)) rank_on_node[nodeName] = 0;
                else rank_on_node[nodeName]++;
                
                if(!rank_of_node.count(nodeName)) rank_of_node[nodeName] = ++nNodes;
                
                rankOnNode[rank] = rank_on_node.at(nodeName);
                rankOfNode[rank] = rank_of_node.at(nodeName);
                
            }
        }
        
        all_reduce<op::sum>(rankOnNode);
        all_reduce<op::sum>(rankOfNode);
        all_reduce<op::sum>(nNodes);
        
        jMPIStructure["rank on node"] = rankOnNode[rank()];
        jMPIStructure["rank of node"] = rankOfNode[rank()];
        jMPIStructure["number of nodes"] = nNodes;
        
        return jMPIStructure;
        
    }
        

}


#endif

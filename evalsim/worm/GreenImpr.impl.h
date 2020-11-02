#ifndef EVALSIM_WORM_GREEN_IMPR_IMPL
#define EVALSIM_WORM_GREEN_IMPR_IMPL

#include "GreenImpr.h"

namespace evalsim {
    
    namespace worm {
        
    
    template<typename Value>
    jsx::value evalsim(ut::wrap<cfg::green_impr::Worm>, jsx::value jParams, jsx::value const& jMeasurements, jsx::value const& jPartition, jsx::value const& jObservables) {
        
        auto const name = cfg::green_impr::Worm::name();
        jsx::value jWorm = jParams(name);
        
        ////Initialization
        
        jParams["hloc"] = ga::read_hloc<Value>("hloc.json");
        
        jParams["operators"] = ga::construct_annihilation_operators<Value>(jParams("hloc"));
        
        double const beta = jParams("beta").real64();
        func::iOmega const iomega(beta); auto const oneBody = jsx::at<io::Matrix<Value>>(jParams("hloc")("one body"));
        jsx::value const jHybMatrix = jParams("hybridisation")("matrix");
        
        std::vector<io::cmat> hyb; std::vector<io::Matrix<Value>> hybMoments;
        
        std::tie(hyb, hybMoments) = partition::func::get_hybridisation<Value>(jParams);
        
        
        pi::cout << "Reading greensigma function and computing green's function... " << std::flush;
        
        std::vector<io::cmat> sigmagreen = meas::read_matrix_functions<Value,Fermion>(jMeasurements, jParams, jWorm, jHybMatrix, hyb.size());
        
        auto const size = std::min(sigmagreen.size(), hyb.size());
        std::vector<io::cmat> green(size, io::cmat(jHybMatrix.size(), jHybMatrix.size()));
        func::green::compute_green_from_improved<Value>(jParams, iomega, oneBody, hyb, sigmagreen, green);
        
        mpi::cout << "OK" << std::endl;
        
        
        mpi::cout << "Calculating self-energy ... " << std::flush;
        
        std::vector<io::cmat> self(size, io::cmat(jHybMatrix.size(), jHybMatrix.size()));
        func::green::compute_self_from_improved<Value>(jParams,sigmagreen,green,self);
        
        mpi::cout << "OK" << std::endl;
        
        
        mpi::cout << "Calculating green moments ... " << std::flush;
        
        std::vector<io::Matrix<Value>> greenMoments = func::green::compute_green_moments<Value>(jParams, hybMoments, jPartition, jObservables("partition")("scalar"));
        
        mpi::cout << "OK" << std::endl;
        
        
        mpi::cout << "Calculating self-energy moments ... " << std::flush;
        
        std::vector<io::Matrix<Value>> selfMoments = func::green::compute_self_moments<Value>(jParams, hybMoments, greenMoments);
        
        mpi::cout << "OK" << std::endl;
        
        
        jsx::value jObservablesOut;
        
        mpi::cout << "Adding self-energy high frequency tail ... "  << std::flush;
        
        func::green::add_self_tail(jHybMatrix, iomega, self, selfMoments, hyb.size());
        jObservablesOut["self-energy"] =  func::write_functions(jParams, jHybMatrix, self, selfMoments);
        
        mpi::cout << "Ok" << std::endl;
        
        
        mpi::cout << "Adding green function high frequency tail ... " << std::flush;
        
        func::green::add_green_tail<Value>(jParams, iomega, oneBody, hyb, self, green);
        jObservablesOut["green"] = func::write_functions(jParams, jHybMatrix, green, greenMoments);
        
        mpi::cout << "Ok" << std::endl;
        
        return jObservablesOut;
    }
    
        
    }
    
}


#endif










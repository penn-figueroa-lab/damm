#include <iostream>
#include <fstream>

#include <Eigen/Dense>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/program_options.hpp>

#include "niw.hpp"
#include "niwDir.hpp"
#include "dpmm.hpp"
#include "dpmmDir.hpp"



namespace po = boost::program_options;
using namespace std;
using namespace Eigen;


int main(int argc, char **argv)
{   
    // std::srand(seed);
    // if(vm.count("seed"))
    // seed = static_cast<uint64_t>(vm["seed"].as<int>());
    // uint64_t seed = time(0);
    uint64_t seed = 1671503159;
    boost::mt19937 rndGen(seed);
    

    cout << "Hello Parallel World" << endl;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("number,n", po::value<int>(), "number of data points")
        ("dimension,m", po::value<int>(), "dimension of data points")
        ("input,i", po::value<string>(), "path to input dataset .csv file rows: dimensions; cols: numbers")
        ("output,o", po::value<string>(), "path to output dataset .csv file rows: dimensions; cols: numbers")
        ("iteration,t", po::value<int>(), "Numer of Sampler Iteration")
        ("alpha,a", po::value<double>(), "Concentration value")
        ("init", po::value<int>(), "Number of initial clusters")
        ("base", po::value<int>(), "Base type: 0 euclidean, 1 euclidean + directional")
        ("params,p", po::value< vector<double> >()->multitoken(), "parameters of the base measure")
    ;

    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);   


    if (vm.count("help")) 
    {
        cout << desc << "\n";
        return 1;
    } 


    int T = 0;
    if (vm.count("iteration")) T = vm["iteration"].as<int>();


    double alpha = 0;
    if(vm.count("alpha")) alpha = vm["alpha"].as<double>();
    assert(alpha != 0);

    
    int init_cluster = 0;
    if (vm.count("init")) init_cluster = vm["init"].as<int>();
    assert(init_cluster != 0);


    int num = 0;
    if (vm.count("number")) num = vm["number"].as<int>();
    assert(num != 0);


    int dim = 0;
    if (vm.count("number")) dim = vm["dimension"].as<int>();
    assert(dim != 0);


    int base = 0;
    if(vm.count("base")) base = static_cast<uint64_t>(vm["base"].as<int>());


    if (base==0) 
    {
        int dimParam = dim / 2;
        double nu;
        double kappa;
        VectorXd mu(dimParam);
        MatrixXd Sigma(dimParam, dimParam);   //Matrix variable named in capital
        if(vm.count("params"))
        {
            vector<double> params = vm["params"].as< vector<double> >();
            nu = params[0];
            kappa = params[1];
            for(uint8_t i=0; i<dimParam; ++i)
                mu(i) = params[2+i];
            for(uint8_t i=0; i<dimParam; ++i)
                for(uint8_t j=0; j<dimParam; ++j)
                    Sigma(i,j) = params[2+dimParam+i+dimParam*j];
        }


        int dimData = dimParam;
        MatrixXd Data(num, dimData);              //Matrix variable named in capital
        string pathIn ="";
        if(vm.count("input")) pathIn = vm["input"].as<string>();
        if (!pathIn.compare(""))
        {
            cout<<"please specify an input dataset"<<endl;
            return 1;
        }
        else
        {
            ifstream  fin(pathIn);
            string line;
            vector<vector<string> > parsedCsv;
            while(getline(fin,line))
            {
                stringstream lineStream(line);
                string cell;
                vector<string> parsedRow;
                while(getline(lineStream,cell,','))
                {
                    parsedRow.push_back(cell);
                }
                parsedCsv.push_back(parsedRow);
            }
            fin.close();
            for (uint32_t i=0; i<num; ++i)
                for (uint32_t j=0; j<dimData; ++j)
                    Data(i, j) = stod(parsedCsv[i][j]);
        }


        cout << "Iteration: " << T <<  "; Concentration: " << alpha << endl
            <<"Number: " << num << "; Data Dimension:" << dimData << "; Parameter Dimension:" << dimParam <<endl;


        NIW<double> niw(Sigma, mu, nu, kappa, rndGen);
        DPMM<NIW<double>> dpmm(Data, init_cluster, alpha, niw, rndGen);
        for (uint32_t t=0; t<T; ++t)
        {
            cout<<"------------ t="<<t<<" -------------"<<endl;
            cout << "Number of components: " << dpmm.K_ << endl;

            dpmm.sampleCoefficientsParameters();
            dpmm.sampleLabels();
            dpmm.reorderAssignments();

        }
        
        const VectorXi& z = dpmm.getLabels();
        string pathOut;
        if(vm.count("output")) pathOut = vm["output"].as<string>();
        if (!pathOut.compare(""))
        {
            cout<<"please specify an output data file"<<endl;
            exit(1);
        }
        else cout<<"Output to "<<pathOut<<endl;
        ofstream fout(pathOut.data(),ofstream::out);
        for (uint16_t i=0; i < z.size(); ++i)
            fout << z[i] << endl;
        fout.close();

        return 0;
    }
    else if (base==1)
    {
        int dimMu = dim;
        int dimCov = dim - 1;
        double nu;
        double kappa;
        VectorXd mu(dimMu);
        MatrixXd Sigma(dimCov, dimCov);   //Matrix variable named in capital
        if(vm.count("params"))
        {
            vector<double> params = vm["params"].as< vector<double> >();
            nu = params[0];
            kappa = params[1];
            for(uint8_t i=0; i<dimMu; ++i)
                mu(i) = params[2+i];
            for(uint8_t i=0; i<dimCov; ++i)
                for(uint8_t j=0; j<dimCov; ++j)
                    Sigma(i,j) = params[2+dimMu+dimCov*i+j];
        }


        int dimData = dim;
        MatrixXd Data(num, dimData);              //Matrix variable named in capital
        string pathIn ="";
        if(vm.count("input")) pathIn = vm["input"].as<string>();
        if (!pathIn.compare(""))
        {
            cout<<"please specify an input dataset"<<endl;
            return 1;
        }
        else
        {
            ifstream  fin(pathIn);
            string line;
            vector<vector<string> > parsedCsv;
            while(getline(fin,line))
            {
                stringstream lineStream(line);
                string cell;
                vector<string> parsedRow;
                while(getline(lineStream,cell,','))
                {
                    parsedRow.push_back(cell);
                }
                parsedCsv.push_back(parsedRow);
            }
            fin.close();
            for (uint32_t i=0; i<num; ++i)
                for (uint32_t j=0; j<dimData; ++j)
                    Data(i, j) = stod(parsedCsv[i][j]);
        }


        cout << "Iteration: " << T <<  "; Concentration: " << alpha << endl
            <<"Number: " << num << "; Data Dimension:" << dimData << "; Parameter Dimension:" << dimCov <<endl;


        NIWDIR<double> niwDir(Sigma, mu, nu, kappa, rndGen);
        DPMMDIR<NIWDIR<double>> dpmmDir(Data, init_cluster, alpha, niwDir, rndGen);

        for (uint32_t t=0; t<T; ++t)
        {
            cout<<"------------ t="<<t<<" -------------"<<endl;
            cout << "Number of components: " << dpmmDir.K_ << endl;
            dpmmDir.sampleCoefficientsParameters();
            dpmmDir.sampleLabels();
            dpmmDir.reorderAssignments();
            if (t==100)
            {
                vector<vector<int>> indexLists = dpmmDir.getIndexLists();
                for (int l=0; l<indexLists.size(); ++l)
                {
                    dpmmDir.splitProposal(indexLists[l]);
                    // dpmmDir.reorderAssignments();
                }
            }
        }
        
        const VectorXi& z = dpmmDir.getLabels();
        string pathOut;
        if(vm.count("output")) pathOut = vm["output"].as<string>();
        if (!pathOut.compare(""))
        {
            cout<<"please specify an output data file"<<endl;
            exit(1);
        }
        else cout<<"Output to "<<pathOut<<endl;
        ofstream fout(pathOut.data(),ofstream::out);
        for (uint16_t i=0; i < z.size(); ++i)
            fout << z[i] << endl;
        fout.close();

        return 0;
    }
    
    // boost::random::uniform_int_distribution<> uni_(0, num-1);
    // for (uint32_t t=0; t<T; ++t)
    // {
    //     // cout<<"------------ t="<<t<<" -------------"<<endl;
    //     cout << "Number of components: " << dpmm.K_ << endl;

    //     // vector<vector<int>> indexLists = dpmm.getIndexLists();
    //     // std::cout << indexLists[0].size() << std::endl;
    //     // dpmm.splitProposal(indexLists[0]);

    //     ////----------------testing-------------------------////
    //     // vector<vector<int>> indexLists = dpmm.getIndexLists();
    //     // dpmm.mergeProposal(indexLists[0], indexLists[1]);
    //     // std::cout << dpmm.K_ << std::endl;
    //     // std::cout << dpmm.Pi_ << std::endl;
    //     ////----------------testing-------------------------////
        

    //     // dpmm.sampleCoefficientsParameters();
    //     // dpmm.sampleLabels();
    //     // dpmm.reorderAssignments();

    //     // int KK = dpmm.K_;
    //     // for (int k=0; k<KK; ++k)
    //     // {   
    //     //     vector<vector<int>> indexLists = dpmm.getIndexLists();
    //     //     vector<int> indexList_k = indexLists[k];   
    //     //     int tt = 0;
    //     //     while (tt < 3)
    //     //     {
    //     //         std::cout << indexList_k.size() << std::endl;
    //     //         if (dpmm.splitProposal(indexList_k)==0)
    //     //         break;                    
    //     //         tt++;
    //     //     }
    //     // }
    //     /*
    //     if (t%1000==0 && t<=7000 && t!=0)
    //     {   
    //         int KK = dpmm.K_;
    //         for (int k=0; k<KK; ++k)
    //         {   
    //             vector<vector<int>> indexLists = dpmm.getIndexLists();
    //             vector<int> indexList_k = indexLists[k];   
    //             int tt = 0;
    //             while (tt < 3)
    //             {
    //                 if (dpmm.splitProposal(indexList_k)==0)
    //                 break;                    
    //                 tt++;
    //             }
    //         }
    //     }
    //     */

    //     /*
    //     if (t == 80)
    //     {   
    //         for (int tt=0; tt<10; tt++)
    //         {   
    //             vector<vector<int>> indexLists = dpmm.getIndexLists();
    //             boost::random::uniform_int_distribution<> uni_(0, indexLists.size()-1);
    //             dpmm.mergeProposal(indexLists[uni_(rndGen)], indexLists[uni_(rndGen)]);
    //         }
    //     }
    //     */
    // }
    
    // /*
    // const VectorXi& z = dpmm.getLabels();
    // string pathOut;
    // if(vm.count("output")) pathOut = vm["output"].as<string>();
    // if (!pathOut.compare(""))
    // {
    //     cout<<"please specify an output data file"<<endl;
    //     exit(1);
    // }
    // else cout<<"Output to "<<pathOut<<endl;
    // ofstream fout(pathOut.data(),ofstream::out);
    // for (uint16_t i=0; i < z.size(); ++i)
    //     fout << z[i] << endl;
    // fout.close();
    // */
}   
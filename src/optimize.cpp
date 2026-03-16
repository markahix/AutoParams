#include "optimize.h"

bool TCJobSuccess()
{
    std::ifstream tcout("opt.out",std::ios::in);
    if (!tcout.is_open())
    {
        return false;
    }
    std::string line;
    while (getline(tcout,line))
    {
        if (line.find("Job finished:") != std::string::npos)
        {
            return true;
        }
        if (line.find("DIE called") != std::string::npos)
        {
            return false;
        }
    }
    return false;
}

void TeraChemOpt(Settings settings)
{
    std::map<std::string,std::string> tc_keywords={  
        {"coordinates",settings.inputfile},
        {"charge",std::to_string(settings.mol_charge)},
        {"spinmult",std::to_string(settings.mol_spin)},
        {"basis","6-31gss"},
        {"method","b3lyp"},
        {"convthre","1e-7"},
        {"threall","1e-14"},
        {"precision","mixed"},
        {"maxit","200"},
        {"scf","diis+a"},
        {"gpus","1"},
        {"gpumem","256"},
        {"scrdir","scr/"},
        {"run","energy"},
        {"resp","yes"}};

    for(std::map<std::string,std::string>::iterator iter = settings.tc_keys.begin(); iter != settings.tc_keys.end(); ++iter)
    {
        std::string k =  iter->first;
        std::string v = iter->second;
        tc_keywords[k] = v;
        if (k == "run")
        {
            std::cout << "Attempted to change runtype from command line call.  Ignoring." << std::endl;
        }
    }
    tc_keywords["run"] = "minimize";

    // Write TeraChem Optimizer input.
    std::stringstream buffer;
    buffer.str("");
    for (std::map<std::string,std::string>::iterator iter = tc_keywords.begin(); iter != tc_keywords.end(); ++iter)
    {
        std::string k =  iter->first;
        std::string v = iter->second;
        buffer << k << '\t' << v <<::std::endl;
    }
    // buffer << "coordinates       " << settings.inputfile << std::endl;
    // buffer << "charge            " << settings.mol_charge << std::endl;
    // buffer << "spinmult          " << settings.mol_spin << std::endl;
    // buffer << "basis             6-31gss" << std::endl;
    // buffer << "method            b3lyp" << std::endl;
    // buffer << "convthre          1e-7" << std::endl;
    // buffer << "threall           1e-14" << std::endl;
    // buffer << "precision         mixed" << std::endl;
    // buffer << "maxit             200" << std::endl;
    // buffer << "scf               diis+a" << std::endl;
    // buffer << "gpus              1" << std::endl;
    // buffer << "gpumem            256" << std::endl;
    // buffer << "scrdir            scr/" << std::endl;
    // buffer << "run               minimize" << std::endl;
    // buffer << "new_minimizer     no" << std::endl;
    // buffer << "min_coordinates   cartesian" << std::endl;

    // buffer << ""<<std::endl;
    // buffer << ""<<std::endl;
    // buffer << ""<<std::endl;
    // buffer << ""<<std::endl;
    // buffer << ""<<std::endl;
    std::ofstream of("opt.in",std::ios::out);
    of << buffer.str();
    of.close();

    // Run TeraChem Optimizer
    silent_shell("terachem -i opt.in 1> opt.out 2> opt.err");

    // Check if optimization finished successfully.
    if (!TCJobSuccess())
    {
        settings.Output("Optimization FAILED.  Continuing with original structure.");
        return;
    }

    // Backup originally submitted pdb.
    buffer.str("");
    buffer << "mv " << settings.inputfile << " Original_PDB.backup; ";
    silent_shell(buffer.str().c_str());

    // Copy final PDB results to original file location.
    std::ifstream optim_pdb("scr/optim.pdb",std::ios::in);
    std::string line;
    buffer.str("");
    while (getline(optim_pdb,line))
    {
        if (line.find("MODEL")!= std::string::npos )
        {
            buffer.str("");
        }
        else if (line.find("ENDMDL")!= std::string::npos )
        {
            continue;
        }
        else
        {
            buffer << line << std::endl;
        }
    }
    optim_pdb.close();
    silent_shell("rm -r scr/ opt.in opt.out opt.err");
    std::ofstream orig_pdb(settings.inputfile,std::ios::out);
    orig_pdb << buffer.str();
    orig_pdb.close();
    return;
}
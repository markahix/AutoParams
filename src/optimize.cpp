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

bool TeraChemOpt(Settings settings, Molecule &mol)
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
        //{"gpumem","256"},
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
    // 2026-09-28: the optimization now runs inside the job directory on the
    // job-directory copy (settings.inputfile there is the centred structure
    // main() just wrote), and its result is read back into `mol`. It used to
    // run in the caller's current directory on the original input, then
    // overwrite that input with the optimized geometry -- after the
    // job-directory copy used for the RESP fit, the mol2 coordinates and the
    // tleap test had already been written, so the optimized geometry was
    // never used for the parameters (the intent, per main.cpp, was always
    // "copy optimized structure to original filename in job_dir").
    std::string curr_path = fs::current_path();
    fs::current_path(settings.job_dir);
    std::ofstream of("opt.in",std::ios::out);
    of << buffer.str();
    of.close();

    // Run TeraChem Optimizer
    buffer.str("");
    if (DEFAULT_TERACHEM_MODULE != "" && settings.USE_MODULES)
    {
        buffer << "module load " << DEFAULT_TERACHEM_MODULE << " && ";
    }
    buffer << "terachem -i opt.in 1> opt.out 2> opt.err";
    
    silent_shell(buffer.str().c_str());
    buffer.str("");

    // Check if optimization finished successfully.
    if (!TCJobSuccess())
    {
        settings.Output("Optimization FAILED.  Continuing with original structure.");
        fs::current_path(curr_path);
        return false;
    }

    // Read the last MODEL of scr/optim.pdb (same atom order as the input).
    std::ifstream optim_pdb("scr/optim.pdb",std::ios::in);
    std::vector<std::array<double,3>> xyz;
    std::string line;
    while (getline(optim_pdb,line))
    {
        if (line.compare(0, 5, "MODEL") == 0)
        {
            xyz.clear();
        }
        else if (is_atom_record(line) && line.size() >= 54)
        {
            xyz.push_back({atof(line.substr(30,8).c_str()),
                           atof(line.substr(38,8).c_str()),
                           atof(line.substr(46,8).c_str())});
        }
    }
    optim_pdb.close();
    if (xyz.size() != mol.atoms.size())
    {
        std::stringstream msg;
        msg << "Optimization finished, but scr/optim.pdb has " << xyz.size() << " atoms (expected "
            << mol.atoms.size() << ").  Continuing with original structure.";
        settings.Error(msg.str());
        settings.Output(msg.str());
        fs::current_path(curr_path);
        return false;
    }
    for (size_t i = 0; i < xyz.size(); i++)
    {
        mol.atoms[i].xx = xyz[i][0];
        mol.atoms[i].yy = xyz[i][1];
        mol.atoms[i].zz = xyz[i][2];
    }
    // opt.in/opt.out/opt.err stay in the job directory as the record of the
    // optimization; only TeraChem's scratch directory is removed.
    silent_shell("rm -rf scr/");
    fs::current_path(curr_path);
    settings.Output("Optimization finished.  The optimized geometry is used for the RESP fit, the mol2 file and the tleap test.");
    return true;
}

#include "parameters.h"

Parameters::Parameters()
{
    // find known parameters file (should be in the include folder)
    std::string exec_folder;
    exec_folder = GetSystemResponse("which autoparams");
    fs::path p = fs::absolute(exec_folder);
    known_params_file = p.parent_path().parent_path() /= "include/known_parameters.dat";
    std::cout << "Loading known parameters from " << known_params_file << std::endl;
    
    if (!fs::exists(known_params_file))
    {
        // If the file cannot be opened, terminate the program (kinda useless otherwise).
        std::cout << "Unable to locate parameter file! Terminating job." << std::endl;
        exit(0);
    }

    // Open and verify the file
    std::ifstream infile;
    infile.open(known_params_file,std::ios::in);
    if ( ! infile.is_open())
    {
        // If the file cannot be opened, terminate the program (kinda useless otherwise).
        std::cout << "Unable to load parameter file! Terminating job." << std::endl;
        exit(0);
    }
    infile.close();
    base_atom_types = {};
    mass_library = {};
    vdw_library = {};
    bond_library = {};
    angle_library = {};
    dihedral_library = {};
}

Parameters::~Parameters()
{

}

void Parameters::InitializeParameterLibrary()
{
    // Declare variables
    std::ifstream infile;
    std::string line;
    std::string current_key;

    if (!fs::exists(known_params_file))
    {
        // If the file cannot be opened, terminate the program (kinda useless otherwise).
        std::cout << "Unable to locate parameter file!" << std::endl;
        exit(0);
    }
    // Open the file (closed below)
    infile.open(known_params_file,std::ios::in);

    if ( ! infile.is_open())
    {
         // If the file cannot be opened, terminate the program (kinda useless otherwise).
        std::cout << "Unable to load parameter file!" << std::endl;
        exit(0);
    }

    // Parse the file line-by-line.
    while (getline(infile,line))
    {
        line = trim_whitespace(line);
        if (line == "")
        {
            continue;
        }
        if (line.substr(0,2) == "//")
        {
            continue;
        }
        if (line.substr(0,1) == "%")
        {
            // Flag encountered
            current_key = line.substr(1,line.size()-1);
            current_key = trim_whitespace(current_key);
        }
        else
        {
            if (current_key == "BASIC_ATOM_TYPES")
            {
                Add_Base_Atom_Type(line);
            }
            if (current_key == "ATOMIC_MASSES")
            {
                Add_Atom_Mass(line);
            }
            if (current_key == "BOND_TYPES")
            {
                Add_Bond(line);
            }
            if (current_key == "ANGLE_TYPES")
            {
                Add_Angle(line);
            }
            if (current_key == "DIHEDRAL_TYPES")
            {
                Add_Dihedral(line);
            }
            if (current_key == "TORSION_TYPES")
            {
                Add_Torsion(line);
            }
            if (current_key == "NONBONDED_TYPES")
            {
                Add_VDW_Parameter(line);
            }
            if (current_key == "DEFAULT_BOND_TYPES")
            {
                Add_Base_Bond(line);
            }
            if (current_key == "DEFAULT_ANGLE_TYPES")
            {
                Add_Base_Angle(line);
            }
            if (current_key == "DEFAULT_DIHEDRAL_TYPES")
            {
                Add_Base_Dihedral(line);                
            }
            if (current_key == "DEFAULT_TORSION_TYPES")
            {
                Add_Base_Torsion(line);
            }
            if (current_key == "DEFAULT_NONBONDED_TYPES")
            {
                Add_Base_VDW(line);
            }
        }        
    }
    infile.close();
}

void Parameters::Add_Base_Atom_Type(std::string line)
{
    // "BASIC_ATOM_TYPES" , 2
    std::string key,value;
    key = line.substr(0,2);
    value = line.substr(2,line.size()-2);
    value = trim_whitespace(value);
    base_atom_types[key] = value;    
}

void Parameters::Add_Atom_Mass(std::string line)
{
    // "ATOMIC_MASSES" , 2
    std::string key,value;
    key = line.substr(0,2);
    value = line.substr(2,line.size()-2);
    mass_library[key] = value;
}

void Parameters::Add_VDW_Parameter(std::string line)
{
    // "NONBONDED_TYPES",2
    std::string key,value;
    key = line.substr(0,2);
    value = line.substr(2,line.size()-2);
    vdw_library[key] = value;
}

void Parameters::Add_Base_VDW(std::string line)
{
    // "DEFAULT_NONBONDED_TYPES",2
    std::string key,value;
    key = line.substr(0,2);
    value = line.substr(2,line.size()-2);
    vdw_library[key] = value;
}

void Parameters::Add_Bond(std::string line)
{
    // "BOND_TYPES",5
    std::string key,value;
    key = line.substr(0,5);
    value = line.substr(5,line.size()-5);
    bond_library[key] = value;
}

void Parameters::Add_Base_Bond(std::string line)
{
    // "DEFAULT_BOND_TYPES",13
    std::string key,value;
    key = line.substr(0,13);
    key = trim_whitespace(key);
    value = " " + line.substr(13,line.size()-13);
    bond_library[key] = value;
}

void Parameters::Add_Angle(std::string line)
{
    // "ANGLE_TYPES",8
    std::string key,value;
    key = line.substr(0,8);
    value = line.substr(8,line.size()-8);
    angle_library[key] = value;
}

void Parameters::Add_Base_Angle(std::string line)
{
    // "DEFAULT_ANGLE_TYPES",21
    std::string key,value;
    key = line.substr(0,21);
    key = trim_whitespace(key);
    value = " " + line.substr(21,line.size()-21);
    angle_library[key] = value;
}

void Parameters::Add_Dihedral(std::string line)
{
    // "DIHEDRAL_TYPES",11
    std::string key,value;
    key = line.substr(0,11);
    value = line.substr(11,line.size()-11);
    if (dihedral_library.find(key) == dihedral_library.end())
    {
        dihedral_library[key] = {value};
    }
    else
    {
        dihedral_library[key].push_back(value);
    }    
}

void Parameters::Add_Base_Dihedral(std::string line)
{
    // "DEFAULT_DIHEDRAL_TYPES",28
    std::string key,value;
    key = line.substr(0,28);
    key = trim_whitespace(key);
    value = " " + line.substr(28,line.size()-28);
    if (dihedral_library.find(key) == dihedral_library.end())
    {
        dihedral_library[key] = {value};
    }
    else
    {
        dihedral_library[key].push_back(value);
    }    


}

void Parameters::Add_Torsion(std::string line)
{
    // "TORSION_TYPES",11

}

void Parameters::Add_Base_Torsion(std::string line)
{
    // "DEFAULT_TORSION_TYPES",28

}

std::string Parameters::FindMissingMass(std::string atom_name)
{
    if (mass_library.find(atom_name) == mass_library.end())
    {
        // if mass is not found, return NewMass with value of "MISSING INFORMATION"
        std::stringstream buffer;
        buffer.str("");
        buffer << std::setw(2) << std::left << atom_name << " MISSING INFORMATION\n";
        return atom_name + buffer.str();
    }
    std::string result = atom_name + mass_library[atom_name] + "\n";
    return result;
}

std::string Parameters::FindMissingBond(std::string a1, std::string a2)
{
    std::stringstream buffer;
    std::string pair,base_key;
    //check if 'a1-a2' exists in the bond library.
    buffer.str("");
    buffer << std::setw(2) << std::left << a1;
    buffer << "-";
    buffer << std::setw(2) << std::left << a2;
    pair = buffer.str();
    base_key = pair;
    if (bond_library.find(pair) != bond_library.end())
    {
        return base_key + bond_library[pair];
    }

    //check if 'a2-a1' exists in the bond library.
    buffer.str("");
    buffer << std::setw(2) << std::left << a2;
    buffer << "-";
    buffer << std::setw(2) << std::left << a1;
    pair = buffer.str();

    if (bond_library.find(pair) != bond_library.end())
    {
        return base_key + bond_library[pair];
    }

    // If neither version of the bond listing are present, obtain basic atom types and generate blurry string
    std::string base1 = base_atom_types[a1];
    std::string base2 = base_atom_types[a2];

    pair = base1+"-"+base2;
    if (bond_library.find(pair) != bond_library.end())
    {
        return base_key + bond_library[pair];
    }

    // flip blurry key string and retry.
    pair = base2+"-"+base1;
    if (bond_library.find(pair) != bond_library.end())
    {
        return base_key + bond_library[pair];
    }
    
    return base_key + " MISSING INFORMATION \n";
}

std::string Parameters::FindMissingAngle(std::string a1, std::string a2, std::string a3)
{
    std::stringstream buffer;
    std::string angle,base_key;
    //check if 'a1-a2-a3' exists in the bond library.
    buffer.str("");
    buffer << std::setw(2) << std::left << a1;
    buffer << "-";
    buffer << std::setw(2) << std::left << a2;
    buffer << "-";
    buffer << std::setw(2) << std::left << a3;
    angle = buffer.str();
    base_key = buffer.str();

    if (angle_library.find(angle) != angle_library.end())
    {
        return base_key + angle_library[angle];
    }

    //check if 'a3-a2-a1' exists in the bond library.
    buffer.str("");
    buffer << std::setw(2) << std::left << a3;
    buffer << "-";
    buffer << std::setw(2) << std::left << a2;
    buffer << "-";
    buffer << std::setw(2) << std::left << a1;
    angle = buffer.str();

    if (angle_library.find(angle) != angle_library.end())
    {
        return base_key + angle_library[angle];
    }

    // No exact match found, expand outward
    // If neither version of the bond listing are present, obtain basic atom types and generate blurry string
    // std::cout << "Unable to find exact match to " << angle1 << " or " << angle2 << std::endl;
    std::string base1 = base_atom_types[a1];
    std::string base2 = base_atom_types[a2];
    std::string base3 = base_atom_types[a3];

    angle = base1 + "-" + base2 + "-" + base3;
    angle = trim_whitespace(angle);
    if (angle_library.find(angle) != angle_library.end())
    {
        return base_key + angle_library[angle];
    }

    angle = base3 + "-" + base2 + "-" + base1;
    angle = trim_whitespace(angle);
    if (angle_library.find(angle) != angle_library.end())
    {
        return base_key + angle_library[angle];
    }
    
    angle = base2;
    if (angle_library.find(angle) != angle_library.end())
    {
        return base_key + angle_library[angle];
    }

    buffer.str("");
    buffer << base_key << " MISSING INFORMATION " << std::endl;

    return buffer.str();
    
}

std::string Parameters::Retrieve_Dihedral(std::string key, std::string base_key)
{
        std::stringstream buffer;
        buffer.str("");
        for (int i=0; i < dihedral_library[key].size();i++)
        {
            buffer << base_key << dihedral_library[key][i] << std::endl;
        }
        return buffer.str();
}

std::string Parameters::FindMissingDihedral(std::string a1, std::string a2, std::string a3, std::string a4)
{
    std::stringstream buffer;
    std::string dihedral,base_key;

    //check if 'a1-a2-a3-a4' exists in the bond library.
    buffer.str("");
    buffer << std::setw(2) << std::left << a1;
    buffer << "-";
    buffer << std::setw(2) << std::left << a2;
    buffer << "-";
    buffer << std::setw(2) << std::left << a3;
    buffer << "-";
    buffer << std::setw(2) << std::left << a4;
    dihedral = buffer.str();
    base_key = dihedral;
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    //check if 'a4-a3-a2-a1' exists in the bond library.
    buffer.str("");
    buffer << std::setw(2) << std::left << a4;
    buffer << "-";
    buffer << std::setw(2) << std::left << a3;
    buffer << "-";
    buffer << std::setw(2) << std::left << a2;
    buffer << "-";
    buffer << std::setw(2) << std::left << a1;
    dihedral = buffer.str();

    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    // No exact match found, expand outward
    // If neither version of the dihedral listing are present, 

    dihedral = "X -" + a2 + "-" + a3 + "-X ";
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    dihedral = "X -" + a3 + "-" + a2 + "-X ";
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    // obtain basic atom types and generate blurry strings
    // std::cout << "Unable to find exact match to " << dihedral1 << " or " << dihedral2 << std::endl;
    std::string base1 = base_atom_types[a1];
    std::string base2 = base_atom_types[a2];
    std::string base3 = base_atom_types[a3];
    std::string base4 = base_atom_types[a4];

    dihedral = base1 + "-" + base2 + "-" + base3 + "-" + base4;
    dihedral = trim_whitespace(dihedral);
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    dihedral = base4 + "-" + base3 + "-" + base2 + "-" + base1;
    dihedral = trim_whitespace(dihedral);
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    dihedral = "X -" + base2 + "-" + base3 + "-" + base4;
    dihedral = trim_whitespace(dihedral);
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    dihedral = "X -" + base3 + "-" + base2 + "-" + base1;
    dihedral = trim_whitespace(dihedral);
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    dihedral = base1 + "-" + base2 + "-" + base3 + "-X ";
    dihedral = trim_whitespace(dihedral);
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }

    dihedral = base4 + "-" + base3 + "-" + base2 + "-X ";
    dihedral = trim_whitespace(dihedral);
    if (dihedral_library.find(dihedral) != dihedral_library.end())
    {
        return Retrieve_Dihedral(dihedral,base_key);
    }


    buffer.str("");
    buffer << base_key << " MISSING INFORMATION " << std::endl;
    return buffer.str();
}

std::string Parameters::FindMissingTorsion(std::string a1, std::string a2, std::string a3, std::string a4)
{
    std::stringstream buffer;
    buffer.str("");
    buffer << std::setw(2) << std::left << a1;
    buffer << "-";
    buffer << std::setw(2) << std::left << a2;
    buffer << "-";
    buffer << std::setw(2) << std::left << a3;
    buffer << "-";
    buffer << std::setw(2) << std::left << a4;
    buffer << " MISSING INFORMATION " << std::endl;
    return buffer.str();
    
}

std::string Parameters::FindMissingNonbonded(std::string a1)
{
    if (vdw_library.find(a1) == vdw_library.end())
    {
        std::stringstream buffer;
        buffer.str("");
        buffer << std::setw(2) << std::left << a1;
        buffer << " MISSING INFORMATION " << std::endl;
        return buffer.str();
    }
    return a1 + vdw_library[a1];
}

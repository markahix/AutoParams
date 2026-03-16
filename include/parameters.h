#ifndef PARAMETERS_H
#define PARAMETERS_H


#include "classes.h"
#include "utilities.h"


// std::map <std::string, std::string> BASIC_ATOM_TYPES;
// std::map <std::string, std::string> mass_library;
// std::vector <std::string> mass_library_keys;
// std::map <std::string, std::string> bond_library;
// std::vector <std::string> bond_library_keys;
// std::map <std::string, std::string> angle_library;
// std::vector <std::string> angle_library_keys;
// std::map <std::string, std::string> dihedral_library;
// std::vector <std::string> dihedral_library_keys;
// std::map <std::string, std::string> vdw_library;
// std::vector <std::string> vdw_library_keys;


class Parameters
{
    private:
        std::string known_params_file;

        // Atoms
        std::map <std::string,std::string> base_atom_types;
        std::map<std::string,std::string> mass_library;
        std::map <std::string,std::string> vdw_library;

        // Bonds
        std::map <std::string,std::string> bond_library;

        // Angles
        std::map <std::string,std::string> angle_library;
        
        // Dihedrals
        std::map <std::string,std::vector<std::string>> dihedral_library;

        // Torsions
        std::map <std::string,std::string> torsions_library;
        
        void Add_Base_Atom_Type(std::string line);
        void Add_Atom_Mass(std::string line);
        void Add_VDW_Parameter(std::string line);
        void Add_Base_VDW(std::string line);

        void Add_Bond(std::string line);
        void Add_Base_Bond(std::string line);

        void Add_Angle(std::string line);
        void Add_Base_Angle(std::string line);

        void Add_Dihedral(std::string line);
        void Add_Base_Dihedral(std::string line);

        void Add_Torsion(std::string line);
        void Add_Base_Torsion(std::string line);

        std::string Retrieve_Dihedral(std::string key, std::string base_key);

    public:
        Parameters();
        ~Parameters();
        void InitializeParameterLibrary();
        std::string FindMissingMass(std::string atom_name);
        std::string FindMissingBond(std::string a1, std::string a2);
        std::string FindMissingAngle(std::string a1, std::string a2, std::string a3);
        std::string FindMissingDihedral(std::string a1, std::string a2, std::string a3, std::string a4);
        std::string FindMissingTorsion(std::string a1, std::string a2, std::string a3, std::string a4);
        std::string FindMissingNonbonded(std::string a1);
};

// std::string FindMissingMass(std::string atom_name);
// std::string FindMissingBond(std::string a1, std::string a2);
// std::string FindMissingAngle(std::string a1, std::string a2, std::string a3);
// std::string FindMissingDihedral(std::string a1, std::string a2, std::string a3, std::string a4);
// std::string FindMissingTorsion(std::string a1, std::string a2, std::string a3, std::string a4);
// std::string FindMissingNonbonded(std::string a1);
// void InitializeParameterLibrary();
#endif
#include "mol2.h"
// Atom Typing:
// Identify element (H, N, C, O, S, Cl, Br, F, I, P, etc.)
// Identify # of bonds vs. max total bonds (count number of bonds that include the atom index, then identify if those are single, double, triple, etc.)
// Identify characteristics (in ring, bridging, etc.)
// Select atom type from base forcefield requested. (if statements, etc.)

std::string process_B(Atom atom, Molecule mol)
{
    if (atom.bonded_to_elements.size() == 4)
    {
        //sp3 boron, return B3
        return "B3";
    }
    //otherwise, sp2 boron, return "B "
    return "B ";

}

std::string process_S(Atom atom, Molecule mol)
{
    if (count_element_in_array(atom.bonded_to_elements,"H") > 0)
    {
        return "SH"; // If there's a hydrogen bonded to the sulfur, it's an "SH" atom type.
    }
    if (count_element_in_array(atom.bonded_to_elements,"S") > 0)
    {
        return "SS"; // If part of a disulfide bond, it's "SS" atom type.
    }
    if (count_element_in_array(atom.bonded_to_elements,"O") > 0)
    {
        return "SO"; // Part of sulfate group, most likely.
    }
    // Otherwise, it's an "S " atom type.
    return "S ";
}

std::string process_C(Atom atom, Molecule mol)
{
    // vector set of values for the incoming atom...
    // number of bonded elements in order? Quicker matching? all properties in an ordered list? how do we do this?
    if (atom.bonded_to_elements.size() == 4)
    {
        // If it's an SP3 carbon, it's "CT"
        return "CT";
    }
    if (atom.bonded_to_elements.size() == 2) //sp carbon.
    {
    // If it's an SP carbon, it's "CY" (bonded to nitrogen) or "CZ" (not bonded to nitrogen).
        if (count_element_in_array(atom.bonded_to_elements,"N") > 0)
        {
            return "CY";
        }
        return "CZ";
    }
    if (atom.bonded_to_elements.size() == 3) //sp2 carbon, possible alkene
    {
        if (count_element_in_array(atom.bonded_to_elements,"H") == 2) // 2H on sp2 carbon means it's terminal alkene
        {
            return "CA";
        }
    }
    // Identify ring structures
    int n_rings = 0;
    bool five_member = false;
    bool six_member = false;

    for (std::vector<int> ring : mol.five_rings)
    {
        if (std::find(ring.begin(), ring.end(), atom.atom_number-1) != ring.end())
        {
            n_rings++;
            five_member = true;
        }
    }
    for (std::vector<int> ring : mol.six_rings)
    {
        if (std::find(ring.begin(), ring.end(), atom.atom_number-1) != ring.end())
        {
            n_rings++;
            six_member = true;
        }
    }

    // If 2 or more rings, it's easy.
    if (n_rings > 1)
    {
        return "CB";
    }

    // If 5-membered ring with 2 nitrogen bonds, it's 
    if (five_member)
    {
        if (count_element_in_array(atom.bonded_to_elements,"N") < 2)
        {
            // if there are fewer than two nitrogens bonded to this carbon, return CC
            return "CC";
        }
        
        // else if there are two or more nitrogens bonded to this carbon, return CQ
        return "CQ";
    }
    if (six_member)
    {
        return "CA";
    }
    // Now we're out of ring structures entirely and should be purely sp2 carbons.
    if (count_element_in_array(atom.bonded_to_elements,"O") == 0)
    {
        return "CD"; // atom in the middle of C=CD-CD=C (conjugated double-bonds)
    }

    if (count_element_in_array(atom.bonded_to_elements,"O") > 0)
    {
        return "C "; // carbonyl group
    }
    
    return "CT"; // in failure, return an sp3 carbon...
}

std::string process_H(Atom atom, Molecule mol)
{
    if (count_element_in_array(atom.bonded_to_elements,"N") == 1)
        return "H ";
    if (count_element_in_array(atom.bonded_to_elements,"O") == 1)
        return "HO";
    if (count_element_in_array(atom.bonded_to_elements,"S") == 1)
        return "HS";
    if (count_element_in_array(atom.bonded_to_elements,"C") == 1)
    {
        // identify carbon type.
        int bonded_atom = atom.bonded_to_indexes[0];
        std::string bonded_atom_type = process_C(mol.atoms[bonded_atom],mol);
        std::map <std::string,std::string> hydrogen_map = {{"CY","HZ"},{"CZ","HZ"},{"CQ","H4"},{"CA","HA"},{"CC","H5"},{"CD","HC"}};
        if (hydrogen_map.count(bonded_atom_type))
        {
            return hydrogen_map[bonded_atom_type];
        }
        return "HC";
    }

    return "HC";    
}

std::string process_N(Atom atom, Molecule mol)
{
    int n_bonds = atom.bonded_to_indexes.size();
    if (n_bonds == 1)
    {
        return "NY";
    }
    if (n_bonds == 3)
    {
        return "N ";
    }
    if (n_bonds == 4)
    {
        return "N3";
    }

    // Identify ring structures
    int n_rings = 0;
    bool five_member = false;
    bool six_member = false;

    for (std::vector<int> ring : mol.five_rings)
    {
        if (std::find(ring.begin(), ring.end(), atom.atom_number-1) != ring.end())
        {
            n_rings++;
            five_member = true;
        }
    }
    for (std::vector<int> ring : mol.six_rings)
    {
        if (std::find(ring.begin(), ring.end(), atom.atom_number-1) != ring.end())
        {
            n_rings++;
            six_member = true;
        }
    }
    if (n_rings > 1)
    {
        return "N*";
    }
    if (n_rings == 1)
    {
        if (five_member)
        {
            if (count_element_in_array(atom.bonded_to_elements,"H") > 0)
            {
                return "NA";
            }
            return "NB";
        }
        if (six_member)
        {
            return "NC";
        }
    }
    if (count_element_in_array(atom.bonded_to_elements,"C") > 0)
    {
        for (int idx : atom.bonded_to_indexes)
        {
            if (mol.atoms[idx].element == "C")
            {
                if (count_element_in_array(mol.atoms[idx].bonded_to_elements,"O") > 0)
                {
                    return "N ";
                }
            }
        }
    }
    return "N*";
}

std::string process_O(Atom atom, Molecule mol)
{
    int n_bonds = atom.bonded_to_elements.size();
    if (count_element_in_array(atom.bonded_to_elements,"H") > 0)  // hydroxyl group oxygen
    {
        return "OH";
    }
    if (count_element_in_array(atom.bonded_to_elements,"P") > 0)  // phosphate oxygen
    {
        if (n_bonds == 1) //terminal oxygen
        {
            return "OP";
        }
        return "O2"; //linking oxygen
    }
    if (count_element_in_array(atom.bonded_to_elements,"S") > 0)  // sulfate oxygen
    {
        return "OS";
    }
    if (count_element_in_array(atom.bonded_to_elements,"C") == 2) // oxygen bound to two carbons is an ester/ether linkage.
    {
        return "OS";
    }
    if ((n_bonds == 1) && (count_element_in_array(atom.bonded_to_elements,"C") == 1)) // if bound to one carbon, check the status of that carbon.
    {
        int b_bond_count = mol.atoms[atom.bonded_to_indexes[0]].bonded_to_indexes.size();
        if (b_bond_count == 4) //sp3 carbon
        {
            return "OH";
        }
        if (b_bond_count == 3) //sp2 carbon
        {
            // how to check between carbonyl and deprotonated OH group?
            return "O "; // carbonyl oxygen
        }
    }        
    return "OH";
}

void AtomTyping(Molecule &mol)
{
    for (Atom &atom : mol.atoms)
    {
        if (atom.element == "S")
        {
            atom.atom_type = process_S(atom,mol);
            continue;
        }
        if (atom.element == "C")
        {
            atom.atom_type = process_C(atom,mol);
            continue;
        }
        if (atom.element == "H")
        {
            atom.atom_type = process_H(atom,mol);
            continue;
        }
        if (atom.element == "N")
        {
            atom.atom_type = process_N(atom,mol);
            continue;
        }
        if (atom.element == "H")
        {
            atom.atom_type = process_H(atom,mol);
            continue;
        }
        if (atom.element == "O")
        {
            atom.atom_type = process_O(atom,mol);
            continue;
        }
        if (atom.element == "B")
        {
            atom.atom_type = process_B(atom,mol);
            continue;
        }
        std::stringstream buf;
        buf.str("");
        buf << atom.element;
        if (atom.element.size() < 2)
            buf << " ";
        atom.atom_type = buf.str();
    }
}


Mol2File::Mol2File(int chg, std::string r_name)
{
    n_atoms = 0;
    n_bonds = 0;
    atoms = "";
    bonds = "";
    charge = chg;
    resname = r_name;
    head_atom = "";
    tail_atom = "";
}

Mol2File::~Mol2File()
{
}

void Mol2File::AddHeadAtom(std::string atomname)
{
    head_atom = atomname;
}

void Mol2File::AddTailAtom(std::string atomname)
{
    tail_atom = atomname;
}

void Mol2File::AddAtom(std::string atomname, double x, double y, double z, std::string atomtype, std::string resname, double respcharge)
{
    n_atoms++;
    std::stringstream buffer;
    buffer.str("");
    buffer << std::setw(7) << std::left << n_atoms << " ";
    buffer << std::setw(4) << std::left << atomname << "     ";
    buffer << std::setw(10) << std::setprecision(4) << std::right << x << " ";
    buffer << std::setw(10) << std::setprecision(4) << std::right << y << " ";
    buffer << std::setw(10) << std::setprecision(4) << std::right << z << " ";
    buffer << std::setw(2) << std::left << atomtype << " ";
    buffer << "         1 ";
    buffer << std::setw(4) << std::left << resname << " ";
    buffer << std::fixed << std::setw(14) << std::setprecision(6) << std::right << respcharge << std::endl;
    atoms += buffer.str();
}

void Mol2File::AddBond(int atom1, int atom2, int order)
{
    n_bonds++;
    std::stringstream buffer;
    buffer.str("");
    buffer << std::setw(6) << std::right << n_bonds << " ";
    buffer << std::setw(5) << std::right << atom1 << " ";
    buffer << std::setw(5) << std::right << atom2 << " ";
    buffer << std::setw(1) << std::right << order << std::endl;
    bonds += buffer.str();
}

void Mol2File::WriteMol2(std::string filename)
{
    std::ofstream ofile(filename,std::ios::out);
    if (! ofile.is_open())
    {
        std::cout << "Unable to write mol2 file." << std::endl;
        return;
    }
    ofile << "@<TRIPOS>MOLECULE" << std::endl;
    ofile << resname << std::endl;
    ofile << std::setw(5) << std::right << n_atoms << " ";
    ofile << std::setw(5) << std::right << n_bonds << "     1     0     0" << std::endl;
    ofile << std::setw(5) << std::right << n_atoms << " ";
    ofile << std::setw(5) << std::right << n_bonds << "     1     0     0" << std::endl;
    ofile << charge << std::endl << std::endl << std::endl;
    ofile << "@<TRIPOS>ATOM" << std::endl;
    ofile << atoms;
    ofile << "@<TRIPOS>BOND" << std::endl;
    ofile << bonds;
    ofile << "@<TRIPOS>SUBSTRUCTURE" << std::endl;
    ofile << "     1   ";
    ofile << std::setw(3) << std::right << resname;
    ofile << "     1 TEMP" << std::endl;
    if (head_atom != "" || tail_atom != "")
    {
        ofile << "@<TRIPOS>HEADTAIL" << std::endl;
        if (head_atom == "")
        {
            ofile << "0 0" << std::endl;
        }
        else
        {
            ofile << head_atom << " 1" << std::endl;
        }
        if (tail_atom == "")
        {
            ofile << "0 0" << std::endl;
        }
        else
        {
            ofile << tail_atom << " 1" << std::endl;
        }
        ofile << "@<TRIPOS>RESIDUECONNECT" << std::endl;
        ofile << "1 ";
        if (head_atom == "")
        {
            ofile << "0";
        }
        else
        {
            ofile << head_atom << " ";
        }
        if (tail_atom == "")
        {
            ofile << "0";
        }
        else
        {
            ofile << tail_atom << " ";
        }
        ofile << "0 0 0 0" << std::endl << std::endl;
    }
    ofile.close();
}
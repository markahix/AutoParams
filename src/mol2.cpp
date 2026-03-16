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
    // Otherwise, it's an "S " atom type.
    return "S ";
}

std::string process_C(Atom atom, Molecule mol)
{
    if (atom.bonded_to_elements.size() == 4)
    {
        // If it's an SP3 carbon, it's "CT"
        return "CT";
    }
    if (atom.bonded_to_elements.size() == 1)
    {
    // If it's an SP carbon, it's "CY" (bonded to nitrogen) or "CZ" (not bonded to nitrogen).
        if (std::find(atom.bonded_to_elements.begin(), atom.bonded_to_elements.end(),"N") != atom.bonded_to_elements.end())
        {
            return "CY";
        }
        return "CZ";
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
    if (count_element_in_array(atom.bonded_to_elements,"P") > 0)
    {
        if (n_bonds == 1)
        {
            return "OP";
        }
        return "O2";
    }
    if (count_element_in_array(atom.bonded_to_elements,"H") > 0)
    {
        return "OH";
    }
    if (count_element_in_array(atom.bonded_to_elements,"C") > 0)
    {
        if (n_bonds == 1)
        {
            return "O ";
        }
        return "OS";
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


/*
from ..utilities import *
from .ParameterUtilities import *
from .AtomTyper import *
from .classes import *
from ..pdbclean import GetResName
import parmed
        
def Generate_Mol2_Bonds(pdbfile):
    pdb = pmd.load_file(pdbfile)
    mol2_bonds = []
    bond_orders = {}
    for key in [atom.name for atom in pdb.atoms]:
        bond_orders[key] = 0
    max_bond_count = {"C":4,"N":3,"O":2,"S":5,"P":6}
    found_bonds = []
    for main_atom in pdb.atoms:
        for bond in main_atom.bonds:
            a1 = bond.atom1
            a2 = bond.atom2
            if sorted([a1.idx+1,a2.idx+1]) in found_bonds:
                continue
            found_bonds.append(sorted([a1.idx+1,a2.idx+1]))
            bond_order = 0
            if any([a1.element_name == "H",a2.element_name == "H"]):
                # All bonds with hydrogens are order 1
                bond_order = 1
            elif any([all([bond.atom2.element_name == "O",len(bond.atom2.bonds)==2]),all([bond.atom1.element_name == "O",len(bond.atom1.bonds)==2])]):
                # All oxygens with two bonds MUST have bond order of 1 for each bond
                bond_order = 1
            elif any([all([a1.element_name == "O", len(a1.bonds)==1,a2.element_name=="C", len(a2.bonds)==3]),all([a2.element_name == "O", len(a2.bonds)==1,a1.element_name=="C", len(a1.bonds)==3])]):
                # All bonds with carbonyl oxygens have a bond order of 2.
                # requires:  sp2 carbon in bond, oxygen with only one bonded atom (how to deal with deprotonated?)
                bond_order = 2
            elif any([all([a2.element_name == "C", len(a2.bonds)==4]),all([a1.element_name == "C", len(a1.bonds)==4])]):
                # All bonds to sp3 carbons must have bond order of 1
                bond_order = 1
            elif any([all([a1.element_name == "C",len(a1.bonds)==3]),all([a2.element_name == "C",len(a2.bonds)==3])]):
                # sp2 carbons have 3 bonded atoms, one of the bonds is bond-order 2
                avail_bonds_1 = max_bond_count[a1.element_name] - bond_orders[a1.name]
                avail_bonds_2 = max_bond_count[a2.element_name] - bond_orders[a2.name]
                if all([avail_bonds_1 > 1,avail_bonds_2 > 1]):
                    bond_order = 2
                else:
                    bond_order = 1
            elif any([a1.element_name == "P",a2.element_name == "P"]):
                avail_bonds_1 = max_bond_count[a1.element_name] - bond_orders[a1.name]
                avail_bonds_2 = max_bond_count[a2.element_name] - bond_orders[a2.name]
                if all([avail_bonds_1 > 1,avail_bonds_2 > 1,any([a1.element_name=="O",a2.element_name=="O"])]): ## Deals with phosphate groups
                    bond_order = 2
                else:
                    bond_order = 1
            else:
                report_failure(f"Unable to assign bond order. {bond}")

            if bond_order > 0:
                atom_list = sorted([a1.idx+1,a2.idx+1])
                mol2bond = [atom_list[0],atom_list[1],bond_order]
                mol2_bonds.append(mol2bond)
                bond_orders[bond.atom1.name] += bond_order
                bond_orders[bond.atom2.name] += bond_order
            else:
                report_failure(f"Unable to assign bond order for {bond}.  Check structure of {pdbfile}")
    # Resort Mol2Bonds to be ordered numerically by first and second atoms
    recheck = True
    while recheck:
        recheck = False
        for i,bond in enumerate(mol2_bonds[:-1]):
            if bond[0] == mol2_bonds[i+1][0]:
                if bond[1] > mol2_bonds[i+1][1]:
                    temp_2 = bond[1]
                    temp_ord = bond[2]

                    bond[1] = mol2_bonds[i+1][1]
                    mol2_bonds[i+1][1] = temp_2
                    bond[2] = mol2_bonds[i+1][2]
                    mol2_bonds[i+1][2] = temp_ord
                    recheck = True
    return mol2_bonds

def PDB_to_Mol2(pdbfile:str,respcharges:list,template:str,charge:int):
    # Initialize Mol2File
    mol2filename = pdbfile.replace(".pdb",".mol2")
    Mol2obj = Mol2File(mol2filename,charge)
    # Generate Atoms,X,Y,Z,Types,Resname,RESPCharges.
    resname = GetResName(pdbfile)
    atomlist = GetPDBAtoms(pdbfile,resname)
    atomtypes = GetAtomTypes(pdbfile,template)
    charges = respcharges
    n_atoms = len(atomlist)
    if not all([ len(atomtypes) == n_atoms,len(charges) == n_atoms]):
        report_failure("Inconsistent number of atomnames,charges, and/or atomtypes.")
    for i in range(n_atoms):
        atomname = atomlist[i][1]
        x = atomlist[i][2]
        y = atomlist[i][3]
        z = atomlist[i][4]
        atomtype = atomtypes[i]
        respcharge = charges[i]
        Mol2obj.AddAtom(atomname,x,y,z,atomtype,resname,respcharge)
    for bond in Generate_Mol2_Bonds(pdbfile):
        Mol2obj.AddBond(bond[0],bond[1],bond[2])
    Mol2obj.WriteFile()

def MAKE_MOL2_FILE(pdbfile,mol2file,respcharges,connections):
    ### Generate the mol2 file.
    GenerateMol2File(pdbfile,mol2file)
    ### Update the mol2 file with resp charges.
    writeMol2(respcharges, mol2file,connections)

def GenerateMol2File(pdbfile,mol2file):
    S.call(f"antechamber -i {pdbfile} -fi pdb -at amber -o {mol2file} -fo mol2 -pf y 1> antechamber.log 2> antechamber.err",shell=True)

def writeMol2(charges, filename, connections):
    lines = open(filename, 'r').readlines()
    i = 0
    start = 0
    end = 0
    for line in lines:
        if "@<TRIPOS>ATOM" in line:
            start = i + 1
        if "@<TRIPOS>BOND" in line:
            end = i
            break
        i = i + 1
    if end - start != len(charges):
        raise RuntimeError("Size of molecule in mol2 file differs from size of molecule in tcout file")
    with open("temp.txt", 'w') as w:
        for line in lines[:start]:
            w.write(line.replace("DU","NB"))
        for line, charge in zip(lines[start:end],charges):
            oldCharge = line.split()[8]
            w.write(line.replace(oldCharge,f"{float(charge):>9.06f}").replace("DU","NB"))
        for line in lines[end:]:
            w.write(line.replace("DU","NB"))
        if connections != ["0","0"]:
            w.write("@<TRIPOS>HEADTAIL\n")
            if connections[0] == "0":
                w.write("0 0\n")
            else:
                w.write(f"{connections[0]} 1\n")
            if connections[1] == "0":
                w.write("0 0\n")
            else:
                w.write(f"{connections[1]} 1\n")
            w.write("@<TRIPOS>RESIDUECONNECT\n")
            w.write(f"1 {connections[0]} {connections[1]} 0 0 0 0\n")
            w.write("\n")
    S.call(f"mv temp.txt {filename}",shell=True)
    
*/

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
/*
    def AddAtom(self,atomname,x,y,z,atomtype,resname,respcharge):
        self.atoms.append(f"{len(self.atoms)+1:>7} {atomname:<4} {float(x):>14.4f} {float(y):>10.4f} {float(z):>10.4f} {atomtype:<2} {1:>10} {resname:<4} {float(respcharge):>14.6f}\n")
*/
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
    buffer << std::setw(14) << std::setprecision(6) << std::right << respcharge << std::endl;
    atoms += buffer.str();
}

void Mol2File::AddBond(int atom1, int atom2, int order)
{
/*
    def AddBond(self,atom1,atom2,order):
        self.bonds.append(f"{len(self.bonds)+1:>6} {atom1:>5} {atom2:>5} {order:>1}\n")
*/
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
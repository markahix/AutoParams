#include "classes.h"

// Identify individual dihedrals that need to be scanned, then process each one individually.  
// If there are multiple instances of the same parameter set (atom name list) in the molecule, do we run multiple scans or just select one and go?
/*
1. Get list of all index-vectors corresponding to dihedrals
2. Identify atom-type sequence for each index-vector
3. If the atom-type sequence is in the list of missing dihedral parameters from tleap output (including reverse order), generate a dihedral scan input sequence for TeraChem optimizations.
3a. If the dihedral is part of a ring structure, just go to the old way?
4.  For a given dihedral, generate a subfolder, copy the PDB file to it, and enter it.  Generate subfolders there for each of the angles between 0-360 at a given interval (10 degrees?), then create a TC input file in each with a constraint included for the dihedral and specific angle.


*/
# AutoParams

Force fields for classical molecular dynamics have been developed for common chemical structures like nucleic acids, amino acids, and some small molecules. 
It is necessary, however, to be able to produce molecule-specific forcefields in situ for uncommon or unique molecules.
These parameters bond/angle/torsion information and partial charges to be useable by software like AMBER or OpenMM. 
AutoParams was developed to analyze a molecular structure, optimize it (optional), calculate RESP partial charges on a by-atom basis, and produce the necessary parameter files for MD simulations of unique molecules. 
Each parameter set is tested internally by AutoParams to ensure proper functionality in part of a larger biological simulation.

### AGIMUS Integration

When installed as part of the AGIMUS suite, AutoParams can be called by FeatureFinder, SolventMixtures, AutoMD, and AutoQuantum.  
It may also call AutoQuantum to obtain additional parameter data if necessary or requested by the end user.

### Citation

[AutoParams: An Automated Web-Based Tool To Generate Force Field Parameters for Molecular Dynamics Simulations](https://doi.org/10.1021/acs.jcim.3c01049)

### Charge generation

AutoParams computes RESP partial charges by shelling out to **TeraChem**, which must be installed and on `PATH` at runtime. 
If TeraChem isn't available, AutoParams logs a clear error up front.

Every atom's charge is validated right after the `.mol2` file is written: each atom is checked individually against a tiny threshold (`|charge| < 1e-6`), and the result is only treated as a failure if **every single atom** comes back near zero. 
A genuinely neutral molecule with real per-atom charges (water: strongly negative O, strongly positive H's, net ~0) is never flagged. 
A molecule with a nonzero formal charge is never flagged on that basis either. 
A molecule where some atoms are legitimately exactly 0 while others are legitimately non-zero is a normal, fine outcome. 
When that happens, `autoparams` now logs `AGIMUS_AUTOPARAMS_ZERO_CHARGES` to stdout, writes an explanatory error to `autoparams.NNNN.err`, and returns a nonzero exit code.

The Overseer-side gate independently re-parses the produced `.mol2` file's charges with the same per-atom check before deciding whether to report the task `ok` and register its output into `library/parameters/`. 

### Non-unique atom names

When atom names in the input PDB repeat, AutoParams renames only the repeats, each to its element plus the lowest number no atom of the molecule uses (an atom whose name was unique keeps it). These new names are the ones the mol2 gives tleap, so it warns, on stderr and in `autoparams.NNNN.out`, listing every rename, for example:

```
WARNING: the input PDB's atom names are not unique; 2 atoms were renamed (atom 4 H1 -> H2, atom 5 H1 -> H3). The mol2 gives tleap the new names: check autoparams.0000/dup.pdb and make sure your molecule's atom names match it. dup.pdb now has the new names; the original is kept as dup_original.pdb.
```

The input is kept as `<stem>_original.pdb` (`_original_2.pdb`, … if that name is taken; nothing is overwritten), and the input is rewritten with only the renamed atoms' names changed. `AGIMUS_AUTOPARAMS_RENAMED_ATOMS renamed:<n> total_atoms:<m> original:<file>` on stdout tells Overseer, which then caches the renamed PDB in `library/parameters/` and says so in the task's message.

### Command Line

Run `autoparams --help` (or `-h`) for the full usage text. 

### Generative AI Disclosure

Generative AI was used for comments and documentation, plus generation of the usage text, argument parser update, and unique atom name handling function update.
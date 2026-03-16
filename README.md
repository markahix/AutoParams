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

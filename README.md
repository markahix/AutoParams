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

### Charge generation and the zero-charge check (2026-09-05)

By default (no `--am1bcc`), AutoParams computes RESP partial charges by shelling out to **TeraChem**, which must be installed, licensed, and on `PATH` at runtime. If TeraChem isn't available, `main.cpp` now logs a clear error up front (`settings.CheckPrograms()` already probes for it into `settings.TC_EXISTS`) instead of silently shelling out to a nonexistent binary and getting no output back.

Whether or not that log message is seen, every atom's charge is validated right after the mol2 file is written (`Charges_Are_Degenerate()`, `autoparams.cpp`): each atom is checked individually against a tiny threshold (`|charge| < 1e-6`), and the result is only treated as a failure if **every single atom** comes back near zero — never based on an aggregate sum, and never compared against the molecule's formal/net charge. A genuinely neutral molecule with real per-atom charges (water: strongly negative O, strongly positive H's, net ~0) is never flagged; a molecule with a nonzero formal charge is never flagged on that basis either; and a molecule where some atoms are legitimately exactly 0 while others are legitimately non-zero is a normal, fine outcome. When that happens, `autoparams` now logs `AGIMUS_AUTOPARAMS_ZERO_CHARGES` to stdout, writes an explanatory error to `autoparams.NNNN.err`, and — as of this fix — actually returns a nonzero exit code (previously `main()` always `return 0`, even after logging a failure, so nothing calling this binary directly could tell success from failure by exit code alone).

The Overseer-side gate (`Overseer/src/main.cpp`, `cmd_finish_autoparams_job()`) independently re-parses the produced mol2 file's charges with the same per-atom check before deciding whether to report the task `ok` and register its output into `library/parameters/`. That's the check that actually matters end-to-end — every AutoParams task, whether run standalone or dispatched via AutoSolvent's/AutoBuild's own blocking-dependency machinery, passes through it before its output can be cached and reused by everything downstream.

**Known gap:** `--am1bcc` (meant to request antechamber's AM1-BCC charge method as a TeraChem-free fallback) is accepted on the command line but has no charge source wired into `main()`'s control flow — `Generate_Mol2_File()`'s antechamber `-c bcc` invocation exists in `autoparams.cpp` but is dead code, never called. Passing `--am1bcc` today just skips the TeraChem/RESP branch entirely and produces the same all-zero-charge result (now caught by the checks above, at least, rather than silently cached). Wiring AM1-BCC up as a real, working fallback is still open.
